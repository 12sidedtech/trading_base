#ifndef __INCLUDED_MEGAQUEUE_MEGAQUEUE_H__
#define __INCLUDED_MEGAQUEUE_MEGAQUEUE_H__

#include <tsl/cal.h>
#include <tsl/result.h>
#include <stdint.h>

/**
 * A megaqueue filename is of the form "megaqueue_[qdesc]"
 */
#define MEGAQUEUE_NAME_PREFIX       "megaqueue_"

/**
 * Contents of the header page of the Megaqueue
 */
struct megaqueue_header {
    /** Head of the Megaqueue, current position */
    uint64_t head CAL_CACHE_ALIGNED;
    /** Tail of the Megaqueue for the sensitive listener */
    uint64_t tail CAL_CACHE_ALIGNED;
    /** Deletion pointer for the Megaqueue */
    uint64_t _delete CAL_CACHE_ALIGNED;
    /* PID of the process that is the producer */
    uint64_t producer_pid CAL_CACHE_ALIGNED;
    /* Size of an object in the Megaqueue */
    uint64_t object_size;
    /* Maximum count of objects in the Megaqueue */
    uint64_t object_count;
} CAL_CACHE_ALIGNED;

struct megaqueue {
    /** Size of an object in the megaqueue, in bytes */
    uint32_t object_size;
    /** Count of objects in the megaqueue */
    uint32_t object_count;
    /** Start of the writable megaqueue region */
    void *writable_start;
    /** The entire shared memory region */
    void *region;
    /** The size of the full mapped region */
    size_t region_size;
    /** Shared memory region name */
    char *rgn_name;

    /* The megaqueue header, used in initialization of a consumer */
    struct megaqueue_header *hdr;

    /** The file descriptor used to map the region */
    int fd;
};

#define MEGAQUEUE_SAFE_INIT_EMPTY { .writable_start = NULL, .region = NULL, .region_size = NULL, .rgn_name = NULL, .hdr = NULL, .fd = -1 }

#define MEGAQUEUE_EMPTY(x) \
    do { \
        (x)->writable_start = NULL; \
        (x)->region = NULL; \
        (x)->region_size = 0; \
        (x)->rgn_name = NULL; \
        (x)->hdr = NULL; \
        (x)->fd = -1; \
    } while (0)

aresult_t megaqueue_open(struct megaqueue *queue,
                         int mode,
                         const char *queue_name,
                         size_t obj_size,
                         size_t obj_count);

aresult_t megaqueue_close(struct megaqueue *queue, int unlink);

/* Internal functions for managing the megaqueue */
static inline
aresult_t megaqueue_advance(struct megaqueue *queue);

static inline
aresult_t megaqueue_next_slot(struct megaqueue *queue, void **slot);

static inline
aresult_t megaqueue_read_next_slot(struct megaqueue *queue, void **slot);

/* WARNING: megaqueue_read_advance SHOULD NEVER BE USED ALONGSIDE megaqueue_delete_advance! */
static inline
aresult_t megaqueue_read_advance(struct megaqueue *queue);

static inline
aresult_t megaqueue_read_only_advance(struct megaqueue *queue);

static inline
aresult_t megaqueue_delete_advance(struct megaqueue *queue);

#include <tsl/megaqueue/megaqueue_priv.h>

#endif /* __INCLUDED_MEGAQUEUE_MEGAQUEUE_H__ */

