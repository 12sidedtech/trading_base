#include <tsl/alloc/logalloc_hugepage.h>

#include <tsl/logalloc.h>
#include <tsl/errors.h>
#include <tsl/diag.h>
#include <tsl/assert.h>

#include <sys/mman.h>

static
aresult_t logalloc_hugepage_alloc(struct logalloc_params *prm, void **prgn_ptr, size_t length)
{
    aresult_t ret = A_OK;

    void *rgn_ptr = NULL;

    TSL_ASSERT_ARG(NULL != prm);
    TSL_ASSERT_ARG(NULL != prgn_ptr);
    TSL_ASSERT_ARG(0 != length);

    if (MAP_FAILED == (rgn_ptr = mmap(NULL, length, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE| MAP_ANONYMOUS | MAP_HUGETLB | MAP_NORESERVE | MAP_POPULATE, -1, 0)))
    {
        PDIAG("Failed to mmap a huge page region of %zu bytes.", length);
        ret = A_E_NOMEM;
        goto done;
    }

    /* Attempt to lock the region in memory */
    if (0 > mlock(rgn_ptr, length)) {
        PDIAG("WARNING: was not able to mlock this hugepage region.");
    }

    *prgn_ptr = rgn_ptr;

done:
    return ret;
}

static
aresult_t logalloc_hugepage_free(struct logalloc_params *prm, void **prgn_ptr, size_t length)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(NULL != prm);
    TSL_ASSERT_ARG(NULL != prgn_ptr);
    TSL_ASSERT_ARG(NULL != *prgn_ptr);
    TSL_ASSERT_ARG(0 != length);

    if (0 > munmap(*prgn_ptr, length)) {
        PDIAG("Unexpected failure while freeing hugepage region. Aborting.");
        goto done;
    }

    *prgn_ptr = NULL;

done:
    return ret;
}

static
struct logalloc_params _logalloc_hugepage_allocator = {
    .alloc = logalloc_hugepage_alloc,
    .free = logalloc_hugepage_free,
    .max_alloc = (size_t)-1,
};

struct logalloc_params *logalloc_hugepage_allocator = &_logalloc_hugepage_allocator;

