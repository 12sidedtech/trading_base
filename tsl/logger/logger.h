#ifndef __INCLUDED_LOGGER_LOGGER_H__
#define __INCLUDED_LOGGER_LOGGER_H__

#include <tsl/rbtree.h>
#include <tsl/list.h>

#include <stdint.h>

struct logger_endpoint;

/**
 * Function pointer for function to be called when the logger message is returned
 * to the initiating thread.
 */
typedef aresult_t (*logger_payload_release_func_t)(void *payload);

struct logger_message {
    /**
     * Message timestamp
     */
    uint64_t timestamp;
    /**
     * Function called to release the state associated with this message on return
     * to initiating thread.
     */
    logger_payload_release_func_t release;
    /**
     * The payload to be emitted.
     */
    void *payload;
    /**
     * The type of message
     */
    unsigned int message_type;
} CAL_CACHE_ALIGNED;

typedef aresult_t (*logger_message_emit_func_t)(void *state, void *payload);

struct logger_message_handler {
    unsigned int message_type;
    void *state;
    logger_message_emit_func_t dispatch;
    struct rb_tree_node tnode;
    struct list_entry lnode;
};

aresult_t logger_initialize(struct logger_endpoint **ep, unsigned int queue_depth);
aresult_t logger_add_message_handler(struct logger_endpoint *ep, unsigned int message_type, logger_message_emit_func_t func);

#endif /* __INCLUDED_LOGGER_LOGGER_H__ */

