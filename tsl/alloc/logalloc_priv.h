#ifndef __INCLUDED_TSL_ALLOC_LOGALLOC_PRIV_H__
#define __INCLUDED_TSL_ALLOC_LOGALLOC_PRIV_H__

#include <tsl/result.h>
#include <tsl/cal.h>

struct logalloc {
    /**
     * The allocator region
     */
    void *rgn;

    /**
     * The current log head, in cells from start of rgn
     */
    size_t log_head;

    /**
     * The cell size, in bytes
     */
    size_t cell_size;

    /**
     * The region size, in bytes
     */
    size_t region_size;

    /**
     * Parameterization of this log-structured allocator.
     */
    struct logalloc_params *params;
} CAL_CACHE_ALIGNED;

/**
 * Private structure inserted at the head of each logalloc cell.
 */
struct logalloc_cell_header {
    /**
     * The number of cells this region represents, contiguously. This does not
     * change until the log head reaches this cell.
     *
     * If nr_cells is 0, this takes on the special meaning indicating that the
     * remainder of the region, up to region_size is free.
     */
    uint8_t nr_cells;

    /**
     * The current reference count of this region. Maximum number of references
     * is 255 before the allocator will cause a panic.
     */
    uint8_t refcnt;
} CAL_PACKED;

#endif /* __INCLUDED_TSL_ALLOC_LOGALLOC_PRIV_H__ */

