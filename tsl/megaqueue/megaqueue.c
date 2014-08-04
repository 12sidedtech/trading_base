/*
  Copyright (c) 2014, 12Sided Technology, LLC
  Author: Phil Vachon <pvachon@12sidedtech.com>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  - Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <tsl/megaqueue/megaqueue.h>

#include <tsl/errors.h>
#include <tsl/diag.h>
#include <tsl/assert.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <ck_pr.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define MB(x) KB((x) * 1024)
#define KB(x) ((x) * 1024)


aresult_t megaqueue_close(struct megaqueue *queue, int unlink)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(NULL != queue);

    if (NULL != queue->region && 0 < queue->region_size) {
        if (munmap(queue->region, queue->region_size) < 0) {
            PDIAG("An error occurred while munmap()ing the megaqueue region.");
        }
        queue->region = NULL;
        queue->region_size = 0;
    }

    if (queue->fd > -1) {
        close(queue->fd);
        queue->fd = -1;
    }

    if (unlink) {
        shm_unlink(queue->rgn_name);
    }

    if (NULL != queue->rgn_name) {
        free(queue->rgn_name);
        queue->rgn_name = NULL;
    }

    memset(queue, 0, sizeof(struct megaqueue));

    return ret;
}

static
aresult_t megaqueue_prepare_superblock(void *base, size_t obj_size, size_t obj_count)
{
    aresult_t ret = A_OK;

    struct megaqueue_header *hdr = base;

    TSL_ASSERT_ARG(NULL != base);

    hdr->head = 0;
    hdr->producer_pid = getpid();
    hdr->object_size = obj_size;
    hdr->object_count = obj_count;

    return ret;
}

/**
 * Destructively pre-fault an entire region of memory. Writes a value to the region,
 * at the beginning of each page. Do not run on initialized pages.
 *
 * \param base The base address to prefault from.
 * \param bytes The number of bytes to prefault, will be rounded to containing page
 * \param page_size Size of a page, in bytes. Must be a power of 2, obviously!
 *
 * \return A_OK on success, an error code otherwise
 */
static
aresult_t __megaqueue_prefault_range(void *base, size_t bytes, size_t page_size)
{
    aresult_t ret = A_OK;
    size_t page_mask = ~(page_size - 1);
    size_t page_base = page_size - 1;
    size_t page_count = ((bytes - 1) & page_mask)/page_size;

    TSL_ASSERT_ARG(0 < bytes);
    TSL_ASSERT_ARG(NULL != base);
    TSL_ASSERT_ARG(0 == ((size_t)base & page_base));

    DIAG("Prefaulting %zu pages (%zu bytes, base is %p, page size %zu bytes, msk = %zx base = %zx)", page_count, bytes, base, page_size, page_mask, page_base);

    for (size_t i = 0; i < page_count; i++) {
        size_t page_offs = i * page_size;

        /* Touch the page */
        *(uint32_t *)(base + page_offs) = 0xdead;
    }

    return ret;
}

/**
 * Open the specified megaqueue with the given O_ constant flags.
 * \param queue The queue information target
 * \param mode The mode (see the O_ constants passed to open(2))
 * \param queue_name The human-readable name of the queue in question.
 * \param obj_size Size of an object living in the queue (must be rounded to nearest power of 2)
 * \param obj_count Count of objects in the queue
 * \return A_OK on success, an error code otherwise.
 *
 * \note Due to limitations in how hugetlbfs works, MADV_REMOVE is not
 *       supported on huge-page mappings. As such, we use standard shm for
 *       megaqueues, because we can punch holes in the VMA quite readily.
 *
 * \see struct megaqueue
 * \see megaqueue_consumer_open
 * \see megaqueue_producer_open
 */
aresult_t megaqueue_open(struct megaqueue *queue,
                         int mode,
                         const char *queue_name,
                         size_t obj_size,
                         size_t obj_count)
{
    aresult_t ret = A_OK;
    char *queue_name_alloc = NULL;
    int qfd = -1;
    void *mapping = MAP_FAILED;
    int prot = PROT_READ;
    size_t page_size = getpagesize();
    size_t queue_size = obj_size * obj_count + page_size;

    TSL_ASSERT_ARG(NULL != queue);
    TSL_ASSERT_ARG(NULL != queue_name);
    TSL_ASSERT_ARG(0 != strlen(queue_name));

    /* Generate the filename for the queue */
    asprintf(&queue_name_alloc, "/%s%s", MEGAQUEUE_NAME_PREFIX, queue_name);

    if (NULL == queue_name_alloc) {
        PDIAG("Unable to generate queue filename.");
        ret = A_E_NOMEM;
        goto done;
    }

    DIAG("Opening Megaqueue '%s'", queue_name_alloc);

    /* Attempt to open the given queue for the specified access mode */
    if ((qfd = shm_open(queue_name_alloc, mode, 0666)) < 0) {
        PDIAG("Failed to open queue '%s' with mode 0x%08x", queue_name_alloc, (unsigned int)mode);
        ret = A_E_INVAL;
        goto done;
    }

    if (mode & O_CREAT) {
        /* ftruncate(2) the region of memory */
        if ((ftruncate(qfd, queue_size)) < 0) {
            PDIAG("Failed to ftruncate(2) the shm fd.");
            ret = A_E_INVAL;
            goto done;
        }
    }

    /* Map the queue into memory */
    if (((O_RDWR & mode) || (O_WRONLY & mode))) {
        prot |= PROT_WRITE;
    }

    if ((mapping = mmap(NULL, queue_size, prot, MAP_SHARED, qfd, 0)) == MAP_FAILED) {
        PDIAG("Failed to mmap(2) shared memory region.");
        ret = A_E_INVAL;
        goto done;
    }

    /* If this was a queue that was created explicitly, invalidate up to the first 512 MB (or the entire queue) */
    if (O_CREAT & mode) {
        size_t prefetch = queue_size > MB(512) ? MB(512) : ((queue_size + page_size - 1));
        prefetch = (prefetch + page_size - 1) & ~(page_size - 1);
        DIAG("Prefetching %zu bytes of megaqueue", prefetch);
        if (madvise(mapping, prefetch, MADV_WILLNEED) < 0) {
            /* Not strictly required, but performance will suffer */
            PDIAG("WARNING: failed to madvise(2) for MADV_WILLNEED.");
        }
        if (AFAILED(__megaqueue_prefault_range(mapping, prefetch, page_size))) {
            PDIAG("Warning: failed to prefault megaqueue directly.");
        }

        megaqueue_prepare_superblock(mapping, obj_size, obj_count);
    } else {
        /* Check the header */
        struct megaqueue_header *hdr = mapping;
        if (hdr->object_size != obj_size || hdr->object_count != obj_count) {
            DIAG("Megaqueue parameters do not match application parameters.");
            ret = A_E_INVAL;
            goto done;
        }
    }

    queue->region = mapping;
    queue->object_size = obj_size;
    queue->object_count = obj_count;
    queue->fd = qfd;
    queue->region_size = queue_size;
    queue->hdr = (struct megaqueue_header *)mapping;
    queue->writable_start = mapping + page_size;
    queue->rgn_name = queue_name_alloc;

done:
    if (AFAILED(ret)) {
        if (NULL != mapping) {
            munmap(mapping, queue_size);
            mapping = NULL;
        }

        if (0 <= qfd) {
            close(qfd);
            qfd = -1;
        }

        if (NULL != queue_name_alloc) {
            free(queue_name_alloc);
            queue_name_alloc = NULL;
        }

    }

    return ret;
}

aresult_t megaqueue_consumer_open(struct megaqueue_consumer **pconsumer,
                                  const char *queue_name,
                                  size_t obj_size,
                                  size_t obj_count)
{
    aresult_t ret = A_OK;

    return ret;
}

aresult_t megaqueue_producer_open(struct megaqueue_producer **pproducer,
                                  const char *queue_name)
{
    aresult_t ret = A_OK;

    return ret;
}

