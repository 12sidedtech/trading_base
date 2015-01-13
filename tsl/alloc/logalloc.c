#include <tsl/logalloc.h>
#include <tsl/alloc/logalloc_priv.h>

#include <tsl/errors.h>
#include <tsl/diag.h>
#include <tsl/assert.h>
#include <tsl/panic.h>
#include <tsl/bits.h>

#include <stdlib.h>
#include <string.h>

#include <ck_pr.h>

/**
 * Default allocator. Nothing fancy, just uses malloc. Probably not ideal.
 */
static
aresult_t __logalloc_default_alloc(struct logalloc_params *prm, void **prgn_ptr, size_t length)
{
    aresult_t ret = A_OK;

    void *rgn_ptr = NULL;

    TSL_ASSERT_ARG(NULL != prm);
    TSL_ASSERT_ARG(NULL != prgn_ptr);
    TSL_ASSERT_ARG(0 != length);

    *prgn_ptr = NULL;

    rgn_ptr = calloc(1, length);
    if (NULL == rgn_ptr) {
        ret = A_E_NOMEM;
        goto done;
    }

    *prgn_ptr = rgn_ptr;

done:
    return ret;
}

static
aresult_t __logalloc_default_free(struct logalloc_params *prm, void **prgn_ptr, size_t length)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(NULL != prm);
    TSL_ASSERT_ARG(NULL != prgn_ptr);
    TSL_ASSERT_ARG(NULL != *prgn_ptr);
    TSL_ASSERT_ARG(0 != length);

    free(*prgn_ptr);
    *prgn_ptr = NULL;

    return ret;
}

static
aresult_t __logalloc_default_max_ref_cnt(struct logalloc_params *prm, void *rgn)
{
    PANIC("Attempted to reference cell region %p one too many times.");
}

static
struct logalloc_params __logalloc_default_params = {
    .alloc = __logalloc_default_alloc,
    .free = __logalloc_default_free,
    .max_refcnt = __logalloc_default_max_ref_cnt,
    .max_alloc = 1ull << 32,
};

aresult_t logalloc_new(struct logalloc **palloc, size_t cell_size, size_t nr_cells, struct logalloc_params *params)
{
    aresult_t ret = A_OK;

    struct logalloc *alloc = NULL;
    size_t rgn_size = 0, cel_size = 0;
    struct logalloc_cell_header *fh = NULL;

    TSL_ASSERT_ARG(NULL != palloc);
    TSL_ASSERT_ARG(0 != cell_size);
    TSL_ASSERT_ARG(0 != nr_cells);

    cel_size = tsl_round_up_2_64(cell_size + sizeof(struct logalloc_cell_header));
    rgn_size = cel_size * nr_cells;

    alloc = calloc(1, sizeof(*alloc));
    if (NULL == alloc) {
        ret = A_E_NOMEM;
        DIAG("Out of memory.");
        goto done;
    }

    if (NULL == params) {
        alloc->params = &__logalloc_default_params;
    } else {
        alloc->params = params;
    }

    if (AFAILED(ret = alloc->params->alloc(alloc->params, &alloc->rgn, rgn_size))) {
        goto done;
    }

    alloc->region_size = rgn_size;
    alloc->cell_size = cel_size;
    alloc->log_head = 0;

    DIAG("Creating new Log Structured Allocator with %zu cells of %zu bytes, in a region of %zu bytes", rgn_size/cel_size, cel_size, rgn_size);

    /* Set the first cell's header to 0 */
    fh = alloc->rgn;
    fh->nr_cells = 0;
    fh->refcnt = 0;

    *palloc = alloc;
done:
    if (AFAILED(ret)) {
        if (NULL != alloc) {
            free(alloc);
            alloc = NULL;
        }
    }
    return ret;
}

aresult_t logalloc_delete(struct logalloc **palloc)
{
    aresult_t ret = A_OK;

    struct logalloc *alloc = NULL;

    TSL_ASSERT_ARG(NULL != palloc);
    TSL_ASSERT_ARG(NULL != *palloc);

    alloc = *palloc;

    if (AFAILED(ret = alloc->params->free(alloc->params, &alloc->rgn, alloc->region_size))) {
        goto done;
    }

    memset(alloc, 0, sizeof(*alloc));

    free(alloc);

    *palloc = NULL;

done:
    return ret;
}

aresult_t logalloc_alloc(struct logalloc *alloc, size_t size, void **pptr)
{
    aresult_t ret = A_OK;

    struct logalloc_cell_header *ch = NULL;
    size_t nr_cells = 0;
    size_t req_cells = 0;
    size_t i = 0;
    size_t last_cell_count = 0;
    size_t working_log_head = 0;

    TSL_ASSERT_ARG(NULL != alloc);
    TSL_ASSERT_ARG(0 != size);
    TSL_ASSERT_ARG(NULL != pptr);

    *pptr = NULL;

    nr_cells = alloc->region_size / alloc->cell_size;

    /* Determine number of cells needed to fill request */
    req_cells = (size + alloc->cell_size - 1)/alloc->cell_size;

    /* We allow a maximum allocation of 255 cells before we return a no-mem */
    if (CAL_UNLIKELY(req_cells > 255 || req_cells > nr_cells)) {
        ret = A_E_NOMEM;
        goto done;
    }

    /* Check if the cell at the log head is available; if it isn't, we're out of memory */
    ch = alloc->rgn + (alloc->log_head * alloc->cell_size);

    if (CAL_UNLIKELY(ck_pr_load_char((char *)&ch->refcnt) != 0)) {
        ret = A_E_NOMEM;
        goto done;
    }

    /* Sanity check - if the region size requested requires more cells than the number available in the
     * queue from the current position, wrap implicitly to 0.
     *
     * NOTE: This isn't the friendliest algorithm, but it's necessary if you try to shove large objects
     * into the end of the log. We're assuming your log is fairly large, right?
     */
    if (CAL_UNLIKELY(req_cells > nr_cells - alloc->log_head)) {
        ch = alloc->rgn + (alloc->log_head * alloc->cell_size);
        ch->refcnt = 0;
        ch->nr_cells = nr_cells - alloc->log_head;
        working_log_head = 0;
    } else {
        working_log_head = alloc->log_head;
    }

    ch = alloc->rgn + (working_log_head * alloc->cell_size);

    /* Attempt to gather cells from local free cells relative to our working log head */
    for (i = 0; i < req_cells;) {
        struct logalloc_cell_header *chdr = NULL;

        chdr = alloc->rgn + ((working_log_head + i) * alloc->cell_size);

        if (chdr->refcnt != 0) {
            break;
        }

        last_cell_count = chdr->nr_cells;

        if (chdr->nr_cells == 0) {
            /* We know that we're free to the end of the memory region from where we started
             * looking, so we can mark that we have from log_head to end of the log cells free.
             */
            i = nr_cells - working_log_head;
            break;
        }

        i += chdr->nr_cells;
    }

    /* We can't allocate enough cells, so break */
    if (i < req_cells) {
        ret = A_E_NOMEM;
        goto done;
    }

    /* Otherwise, fill in our new entry */
    ch->refcnt = 1;
    ch->nr_cells = req_cells;

    *pptr = ((void *)ch) + sizeof(*ch);

    /* Update the log head with our new calculated log head */
    alloc->log_head = (working_log_head + req_cells) % nr_cells;

    /* And mark new regions of free cells starting containing whatever remainder we had, if any */
    if (req_cells < i) {
        size_t rem = last_cell_count == 0 ? 0 : i - req_cells;
        size_t offs = 0;

        do {
            size_t val = (rem > 255 ? 255 : rem);
            ch = alloc->rgn + ((alloc->log_head + offs) * alloc->cell_size);
            /* Write out an indicator */
            ch->refcnt = 0;
            ch->nr_cells = val;
            rem -= val;
        } while (rem > 0);
    }

done:
    return ret;
}

aresult_t logalloc_reference(void *ptr)
{
    aresult_t ret = A_OK;

    struct logalloc_cell_header *ch = NULL;

    TSL_ASSERT_ARG(NULL != ptr);

    ch = ptr - sizeof(*ch);

    if (CAL_UNLIKELY(ch->refcnt == 255 || ch->refcnt == 0)) {
        ret = A_E_BUSY;
        goto done;
    }

    ck_pr_inc_char((char *)&ch->refcnt);

done:
    return ret;
}

aresult_t logalloc_free(void **pptr)
{
    aresult_t ret = A_OK;

    struct logalloc_cell_header *ch = NULL;
    void *ptr = NULL;

    TSL_ASSERT_ARG(NULL != pptr);
    TSL_ASSERT_ARG(NULL != *pptr);

    ptr = *pptr;

    ch = ptr - sizeof(*ch);

    if (CAL_UNLIKELY(ch->refcnt == 0)) {
        ret = A_E_INVAL;
        goto done;
    }

    ck_pr_dec_char((char *)&ch->refcnt);

done:
    return ret;
}

aresult_t logalloc_prepare_region(struct logalloc *alloc, size_t size_hint, void **pptr, size_t *psize)
{
    aresult_t ret = A_OK;

    size_t nr_cells = 0, found_cells = 0;
    size_t avail_cells = 0;
    size_t virt_log_head = 0;
    struct logalloc_cell_header *ch = NULL;
    size_t rem_cells = 0, cur_cell = 0;
    bool remainder = false;

    TSL_ASSERT_ARG(NULL != alloc);
    TSL_ASSERT_ARG(0 != size_hint);
    TSL_ASSERT_ARG(NULL != pptr);
    TSL_ASSERT_ARG(NULL != psize);

    nr_cells = (size_hint + alloc->cell_size - 1)/alloc->cell_size;

    if (CAL_UNLIKELY(255 < nr_cells)) {
        nr_cells = 255;
    }

    /* Calculate the total number of cells in the allocator */
    avail_cells = (alloc->region_size / alloc->cell_size) - alloc->log_head;

    if (CAL_UNLIKELY(avail_cells < nr_cells)) {
        ret = A_E_NOMEM;
        goto done;
    }

    /* Check to see if the current log head is "full" */
    ch = alloc->rgn + (alloc->log_head * alloc->cell_size);
    if (ck_pr_load_char((char *)&ch->refcnt) != 0) {
        ret = A_E_NOMEM;
        goto done;
    }

    /* Try wrapping the allocator if the remainder in the allocator is not sufficiently sized for the request */
    if (avail_cells < nr_cells) {
        /* Note: we can get away with this, even if there are occupied cells after the log pointer -- if there's enough
         * cell space free at the base of the log region, we'll just start writing there. If not, we'll fail immediately
         * with an A_E_NOMEM
         */
        virt_log_head = 0;
    } else {
        virt_log_head = alloc->log_head;
    }

    do {
        /* Get the cell header at the current position */
        struct logalloc_cell_header *chdr = alloc->rgn + ((virt_log_head + found_cells) * alloc->cell_size);

        if (ck_pr_load_char((char *)&chdr->refcnt) != 0) {
            break;
        }

        /* Handle the special 0 sentinel values */
        if (chdr->nr_cells == 0) {
            found_cells += (avail_cells - virt_log_head - found_cells);
            remainder = true;
            break;
        } else {
            found_cells += chdr->nr_cells;
        }
    } while (nr_cells < found_cells);

    if (found_cells < nr_cells) {
        /* Couldn't get enough cells to fill the requirement */
        ret = A_E_NOMEM;
        goto done;
    }

    rem_cells = found_cells - nr_cells;
    alloc->log_head = virt_log_head;

    /* Populate a placeholder for the new region */
    ch = alloc->rgn + (virt_log_head * alloc->cell_size);
    ch->nr_cells = nr_cells;
    ch->refcnt = 0;
    cur_cell = nr_cells;

    /* OK, now rearrange the cell boundaries beyond our initial allocation */
    if (CAL_UNLIKELY(true == remainder)) {
        /* We found the special 0 sentinel, indicating the entirety of the log is free from the current position.
         * Push the sentinel along to the end of the allocated region
         */
        struct logalloc_cell_header *chdr = alloc->rgn + ((virt_log_head + cur_cell) * alloc->cell_size);
        chdr->nr_cells = 0;
        chdr->refcnt = 0;
    } else {
        while (rem_cells > 0) {
            size_t nr = rem_cells > 255 ? 255 : rem_cells;
            struct logalloc_cell_header *chdr = alloc->rgn + ((virt_log_head + cur_cell) * alloc->cell_size);
            chdr->nr_cells = nr;
            chdr->refcnt = 0;
            cur_cell += nr;
            rem_cells -= nr;
        }
    }

    *pptr = (void *)ch + sizeof(*ch);
    *psize = nr_cells * alloc->cell_size;

done:
    return ret;
}

aresult_t logalloc_finalize_region(struct logalloc *alloc, size_t size)
{
    aresult_t ret = A_OK;

    struct logalloc_cell_header *ch = NULL;
    size_t nr_cells_used = 0;
    size_t rem_cells = 0;
    size_t total_cells = 0;

    TSL_ASSERT_ARG(NULL != alloc);
    TSL_ASSERT_ARG(0 != size);

    total_cells = alloc->region_size/alloc->cell_size;

    nr_cells_used = (size + alloc->cell_size - 1)/alloc->cell_size;
    ch = alloc->rgn + (alloc->log_head * alloc->cell_size);

    /* Sanity check to make sure we didn't do anything stupid */
    if (CAL_UNLIKELY(nr_cells_used > 255 || nr_cells_used > ch->nr_cells)) {
        ret = A_E_INVAL;
        goto done;
    }

    /* Move log head to the next free region of cells, wrapping if we hit the end */
    alloc->log_head = (alloc->log_head + nr_cells_used) % total_cells;

    /* If the cells consumed in the region are equal to the size of the region, we can short-circuit this */
    if (ch->nr_cells == nr_cells_used) {
        goto done;
    }

    /* Otherwise, we need to place a new cell record at the new log head */
    rem_cells = ch->nr_cells - nr_cells_used;
    ch->nr_cells = nr_cells_used;
    ch->refcnt = 1;

    ch = alloc->rgn + (alloc->log_head * alloc->cell_size);
    ch->refcnt = 0;
    ch->nr_cells = rem_cells;

done:
    return ret;
}

