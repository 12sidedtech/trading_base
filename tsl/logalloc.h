#ifndef __INCLUDED_TSL_LOGALLOC_H__
#define __INCLUDED_TSL_LOGALLOC_H__

#include <tsl/result.h>

/* Forward declarations */
struct logalloc_params;

/**
 * The log-structured allocator handle. A log-structured allocator allows allocation
 * of items from a working head in specified unit chunks. Allocation can be performed
 * from any thread, and deallocation can be performed from any other thread.
 */
struct logalloc;

/**
 * Typedef for managing the logalloc region.
 */
typedef aresult_t (*logalloc_alloc_func_t)(struct logalloc_params *prm, void **rgn_ptr, size_t length);

/**
 * Typedef for a call to be made when a logalloc cell has exceeded its maximum refcnt
 */
typedef aresult_t (*logalloc_max_ref_cnt_func_t)(struct logalloc_params *prm, void *rgn);

/**
 * Parameters descriptive structure, used to alter the behavior of the logalloc on how
 * the memory region is managed.
 *
 * This structure is owned by whoever called logalloc_new, and must exist for the duration
 * of the lifecycle of the logalloc itself.
 */
struct logalloc_params {
    /**
     * Function called to allocate memory.
     */
    logalloc_alloc_func_t alloc;

    /**
     * Function called to free memory. Requires the length of the region as allocated as an argument.
     */
    logalloc_alloc_func_t free;

    /**
     * Function to be called when the reference count for a particular cell exceeds the maximum
     * reference count allowed.
     */
    logalloc_max_ref_cnt_func_t max_refcnt;

    /**
     * The maximum single region allocation supported, in bytes
     */
    size_t max_alloc;
};

/**
 * \brief Create a new logalloc.
 *
 * The allocator will create region_size / cell_size entry allocator that can be used
 * in a log-structured fashion.
 *
 * \param palloc Pointer to the new allocator, passed by reference
 * \param cell_size Size of a cell in the allocator, in bytes. Must be a power of two.
 * \param region_size The size of the allocator arena to be turned into a log-structured
 *                    allocator.
 * \param params The parameters structure used to parameterize the behavior of this allocator.
 *               This can be NULL to use the default behaviors for the logalloc.
 *
 * \return A_OK on success, an error code otherwise
 *
 * \note params MUST live as long as the logalloc instance does, though the logalloc is not
 *       responsible for deallocating the params structure itself. Rather, this must be done
 *       by the caller.
 */
aresult_t logalloc_new(struct logalloc **palloc, size_t cell_size, size_t region_size, struct logalloc_params *params);

/**
 * \brief Destroy a logalloc.
 *
 * Release all resources backing the given logalloc, without regard for whether or not
 * there are any oustanding cells (though the logalloc will complain if it believes there
 * are outstanding cells).
 *
 * \param palloc The allocator to destroy. Set to NULL on success.
 *
 * \return A_OK on success, an error code otherwise
 */
aresult_t logalloc_delete(struct logalloc **palloc);

/**
 * \brief Allocate an object in a logalloc.
 *
 * The object will occupy `CIEL(obj_size / cell_size)` cells.
 *
 * \param alloc The allocator to allocate the object from
 * \param size The size of the object, in bytes
 * \param pptr A pointer to the newly allocated region, returned by reference.
 *
 * \return A_OK on success, an error code otherwise.
 */
aresult_t logalloc_alloc(struct logalloc *alloc, size_t size, void **pptr);

/**
 * \brief Increment the reference count of the given object.
 *
 * Given an object from a logalloc, increment the reference count of this object.
 *
 * \param ptr The pointer to the object that the reference count is to be incremented for.
 *
 * \return A_OK on success, A_E_BUSY if the reference count exteeds the maximum logalloc
 *         reference count, A_E_INVAL if the reference count is currently 0.
 *
 * \note This doesn't really attempt to verify the pointer you've provided is a valid
 *       cell (how can it?), so if you've done something stupid, you will get burned.
 */
aresult_t logalloc_reference(void *ptr);

/**
 * \brief Release the given object. Does not guarantee object will be freed.
 *
 * Decrements the reference count by one for the given object. If the reference count reaches
 * 0, the object will be considered free on the next iteration of the log.
 *
 * \param pptr The pointer to the object to release. Set to NULL for the caller.
 *
 * \return A_OK on success, an error code otherwise.
 *
 * \note This doesn't attempt to verify the pointer you've provided is a valid cell,
 *       so if you pass it a random pointer, expect random behavior.
 */
aresult_t logalloc_free(void **ptr);

/**
 * Get a pointer to the head of a log region, with the maximum size, in bytes, that can be
 * put at that pointer. Useful for socket receive operations on streams.
 *
 * \param alloc The allocator to get the head from
 * \param size_hint A size hint to the allocator to help it determine how many cells the caller
 *        might require.
 * \param pptr The pointer to the head of the log region
 * \param psize The size of the maximum entry that can fill this region
 *
 * \return A_OK on success, an error code otherwise.
 *
 * \note This function attempts to find CEIL(size_hint/cell_size) cells that can meet the estimated
 *       requirement. If the number of cells is excessively large, this will slow your application
 *       down under some circumstances.
 *
 * \note For the unaware, TCP segments are usually maxed out around 1500 bytes.
 */
aresult_t logalloc_prepare_region(struct logalloc *alloc, size_t size_hint, void **pptr, size_t *psize);

/**
 * "Cast" a pointer in a logalloc, closing a pending prepared allocation..
 *
 * \param alloc The allocator in question.
 * \param size The length of the allocation, starting from the current head (returned as a pointer in a
 *        call to logalloc_prepare_region.
 *
 * \return A_OK on success, an error code otherwise
 */
aresult_t logalloc_finalize_region(struct logalloc *alloc, size_t size);


#endif /* __INCLUDED_TSL_LOGALLOC_H__ */

