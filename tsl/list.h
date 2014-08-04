/*
  Copyright (c) 2013, Phil Vachon <phil@cowpig.ca>
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
#ifndef __INCLUDED_TSL_LIST_H__
#define __INCLUDED_TSL_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * Trading Standard Library Generic Linked Lists
 */

#include <tsl/version.h>
#include <tsl/basic.h>

#include <stddef.h>

#define LIST_POISON_NEXT    0xc1c1c1c1
#define LIST_POISON_PREV    0x81818181

/**
 * \struct list_entry
 * Structure that represents a linked list node or a linked list head.
 */
struct list_entry {
    struct list_entry *prev;
    struct list_entry *next;
};

/** \brief Initialize a static list item
 * Initialize a static list item to an empty state in .data
 */
#define LIST_INIT(item) { .prev = &(item), .next = &(item) }

/** \brief Declare a new list head
 */
#define LIST_HEAD(name) struct list_entry name = LIST_INIT(name)

/**
 * \brief Get pointer to containing struct
 * If a list_entry is embedded in another struct, return a pointer to the
 * containing struct.
 */
#define LIST_ITEM(list_ptr, type, member) \
    BL_CONTAINER_OF(list_ptr, type, member)

/**
 * Get the next item from a list
 */
#define LIST_NEXT(list_ptr) ((list_ptr)->next)

/**
 * \brief Iterate through all items in a linked list
 * Iterate through a linked list and populate the provided iterator
 * with the current active list item. Behaves like a generic for
 * loop.
 * \param iterator a struct list_entry pointer for iteration
 * \param head the list head
 * \note Use this macro with care -- it doesn't support assignment
 *       and can unintentionally fuse with other code blocks
 */
#define list_for_each(iterator, head)           \
    for ((iterator) = (head)->next;             \
            ((iterator) != (head));             \
            (iterator) = (iterator)->next)

/**
 * \brief Iterate through all items in a linked list, by type
 * Iterate through a linked list and populate the provided iterator,
 * a pointer of the specified container type, with a pointer to
 * the item in the list.
 * \param iterator an iterator of the type in which the struct list is embedded
 * \param head the struct list_head in question
 * \param member the name of the struct list_head member
 * \note If you want to destroy or remove members from the list, don't use this iterator
 */
#define list_for_each_type(iterator, head, member)                               \
    for (iterator = LIST_ITEM((head)->next, __typeof__(*iterator), member);          \
         &iterator->member != (head);                                            \
         iterator = LIST_ITEM((iterator)->member.next, __typeof__(*iterator), member))

/**
 * \brief Safely iterate through an entire linked list with a cursor.
 * Perform an interation through a linked list without generating a pointer to a countainer
 * type.
 */
#define list_for_each_safe(iter, temp, head) \
    for (iter = (head)->next,                       \
            temp = iter->next;                      \
         iter != (head);                            \
         iter = temp,                               \
            temp = iter->next)

/**
 * \brief Iterate through all items in a linked list, with a cursor.
 * Iterate through all items in a linked list and populate the provided iterator,
 * managing the pointer to the next item in the list using a temporary look-ahead value.
 * This iterator is safe for deletion of items.
 * \param iterator The cursor
 * \param temp A temporary item used to cache the look-ahead
 * \param head The head of the linked list
 * \param member The name of the member
 */
#define list_for_each_type_safe(iterator, temp, head, member) \
    for (iterator = LIST_ITEM((head)->next, __typeof__(*iterator), member), \
            temp = LIST_ITEM(iterator->member.next, __typeof__(*iterator), member); \
         &iterator->member != (head); \
         iterator = temp, temp = LIST_ITEM(temp->member.next, __typeof__(*iterator), member))

/* Initialize a list item to an empty state */
static inline
void list_init(struct list_entry *_new)
{
    _new->next = _new->prev = _new;
}

/* Private function for splicing an item into a list */
static inline
void __list_add(struct list_entry *_new,
                struct list_entry *prev,
                struct list_entry *next)
{
    prev->next = _new;
    _new->prev = prev;
    next->prev = _new;
    _new->next = next;
}

/**
 * \brief Add a new item to the list
 * Adds a new item to the linked list. This is an O(1) operation.
 */
static inline
void list_append(struct list_entry *head, struct list_entry *_new)
{
    __list_add(_new, head->prev, head);
}

/**
 * \brief Add a new item to the list at the front
 * Adds a new item to the linked list at the front. This is an O(1) operation.
 */
static inline
void list_prepend(struct list_entry *head, struct list_entry *_new)
{
    __list_add(_new, head, head->next);
}

/* Private function for removing an item from a list */
static inline
void __list_del(struct list_entry *prev, struct list_entry *next)
{
    prev->next = next;
    next->prev = prev;
}

/**
 * \brief Delete an item from a linked list
 * Delete the specified item from the linked list. This is an O(1) operation.
 */
static inline
void list_del(struct list_entry *del)
{
    __list_del(del->prev, del->next);
    del->next = (void *)LIST_POISON_NEXT;
    del->prev = (void *)LIST_POISON_PREV;
}

/* Private function to splice two linked lists together */
static inline
void __list_splice(struct list_entry *target_head,
                   struct list_entry *first_prev,
                   struct list_entry *first_next,
                   struct list_entry *second_prev,
                   struct list_entry *second_next)
{
    /* Splice the two lists together */
    first_prev->next = second_next;
    second_next->prev = first_prev;

    /* Update the new head and tail with the pointer to the head */
    second_prev->next = target_head;
    first_next->prev = target_head;

    /* Update the target head */
    target_head->prev = second_prev;
    target_head->next = first_next;
}

/**
 * \brief Given two list heads, splice the lists
 * Splice the given two list heads into a single, large list, attached to the
 * specified new head.
 * \note The new head can be the same as one of the original heads
 */
static inline
void list_splice(struct list_entry *target,
                 struct list_entry *first,
                 struct list_entry *second)
{
    __list_splice(target, first->prev,  first->next,
                          second->prev, second->next);
}

/**
 * \brief Determine if a list is empty
 */
static inline
int list_empty(struct list_entry *head)
{
    return !!(head->next == head);
}

static inline
int list_is_last(struct list_entry *head, struct list_entry *item)
{
    return !!(item->next == head);
}

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_TSL_LIST_H__ */

