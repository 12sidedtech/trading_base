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
#ifndef __INCLUDED_TSL_CONFIG_ENGINE_H__
#define __INCLUDED_TSL_CONFIG_ENGINE_H__

#include <tsl/result.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** \file config/engine.h Configuration Engine
 *
 */

/**
 * The type of the value in a configuration atom
 */
enum config_atom_type {
    CONFIG_ATOM_INTEGER,
    CONFIG_ATOM_STRING,
    CONFIG_ATOM_ARRAY,
    CONFIG_ATOM_NESTED,
    CONFIG_ATOM_BOOLEAN,
    CONFIG_ATOM_FLOAT,
    CONFIG_ATOM_NULL
};

/**
 * Structure defining a configuration atom. These are passed in by the
 * caller to a configuration getter/setter, and are solely the responsibility
 * of the caller to manage the lifecycle of. Typically the object can be
 * passed on stack to a getter/setter function without any sort of allocation.
 *
 * Config items are treated as a tree of config objects. Helper functions are
 * provided to access members of a config tree using the specified type cast,
 * but these assume you're operating on a leaf of the tree.
 */
struct config {
    enum config_atom_type atom_type;
    union {
        char *atom_string;
        void *atom_array;
        void *atom_nested;
        int atom_integer;
        bool atom_bool;
        float atom_float;
    };
};

/**
 * Return the length of an array atom, if the specified atom is an array.
 * \param atm The atom to query
 * \param len The length, by reference
 * \return A_OK on success, an error code otherwise
 */
aresult_t config_array_length(struct config *array, size_t *length);

/**
 * Return the value of an array at the given location
 * \param array The array to query
 * \param item The atom to receive the array item
 * \param index The index of the array item
 */
aresult_t config_array_at(struct config *array, struct config *item, size_t index);
aresult_t config_array_at_integer(struct config *array, int *item, size_t index);
aresult_t config_array_at_string(struct config *array, char **item, size_t index);

aresult_t config_get_integer(struct config *cfg, int *val, const char *item_id);
aresult_t config_get_string(struct config *cfg, char **val, const char *item_id);
aresult_t config_get_boolean(struct config *cfg, bool *val, const char *item_id);

#define __CONFIG_GET_PREFIX(which, cfg, tgt, prefix, item_id)           \
    ({  aresult_t __ret = A_OK;                                         \
        char *__xpath = NULL;                                           \
        asprintf(&__xpath, "%s.%s", prefix, item_id);                   \
        if (NULL == __xpath) {                                          \
            __ret = A_E_NOMEM;                                          \
        } else {                                                        \
            __ret = which(cfg, &tgt, __xpath);                          \
            free(__xpath);                                              \
            __xpath = NULL;                                             \
        }                                                               \
        __ret;                                                          \
     })

#define CONFIG_GET_INTEGER_PFX(cfg, tgt, prefix, item_id) \
    __CONFIG_GET_PREFIX(config_get_integer, cfg, tgt, prefix, item_id)

#define CONFIG_GET_STRING_PFX(cfg, tgt, prefix, item_id) \
    __CONFIG_GET_PREFIX(config_get_string, cfg, tgt, prefix, item_id)

#define CONFIG_GET_BOOLEAN_PFX(cfg, tgt, prefix, item_id) \
    __CONFIG_GET_PREFIX(config_get_boolean, cfg, tgt, prefix, item_id)

#define CONFIG_GET_PFX(cfg, tgt, prefix, item_id) \
    __CONFIG_GET_PREFIX(config_get, cfg, tgt, prefix, item_id)

/**
 * Get an item from the specified configuration object.
 * \param cfg Configuration object.
 * \param atm The atom to receive the item information
 * \param item_id The DOM-style name of the item.
 * \return A_OK on success, A_E_NOT_FOUND if item_id does not exist, an error code otherwise
 */
aresult_t config_get(struct config *cfg, struct config *atm, const char *item_id);

/**
 * Create a new configuration engine storage object
 * \param pcfg Pointer to receive the new configuration object.
 * \return A_OK on success, an error code otherwise
 */
aresult_t config_new(struct config **cfg);

/**
 * Parse the given configuration file and add it to the configuration
 * structure.
 * \param cfg The configuration object to add the config to
 * \param filename The file to parse the configuration from
 * \return A_OK on success, an error code otherwise
 */
aresult_t config_add(struct config *cfg, const char *filename);

/**
 * Serialize the current configuration and return it as a string on the
 * heap.
 * \param cfg The configuration to serialize
 * \param config The configuration returned as a serialized JSON object
 * \return A_OK on success, an error code otherwise
 * \note It is the caller's responsibility to free the string. Free it
 *       using standard free(3)
 */
aresult_t config_serialize(struct config *cfg, char **config);

/**
 * Destroy the given configuration object
 */
aresult_t config_delete(struct config **pcfg);

#endif /* __INCLUDED_TSL_CONFIG_ENGINE_H__ */

