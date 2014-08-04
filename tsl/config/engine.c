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
#include <tsl/config/engine.h>

#include <tsl/diag.h>
#include <tsl/assert.h>
#include <tsl/errors.h>

#include <jansson.h>

#include <stdio.h>
#include <string.h>

#define CONFIG_MSG(sev, ident, message, ...) \
    MESSAGE("CONFIG", sev, ident, message, ##__VA_ARGS__)

static
aresult_t __config_from_json(struct config *atom, json_t *json)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(NULL != atom);
    TSL_ASSERT_ARG(NULL != json);

    switch (json_typeof(json)) {
    case JSON_ARRAY:
        atom->atom_type = CONFIG_ATOM_ARRAY;
        atom->atom_array = json;
        break;
    case JSON_INTEGER:
        atom->atom_type = CONFIG_ATOM_INTEGER;
        atom->atom_integer = json_integer_value(json);
        break;
    case JSON_STRING:
        atom->atom_type = CONFIG_ATOM_STRING;
        atom->atom_string = (char *)json_string_value(json);
        break;
    case JSON_TRUE:
        atom->atom_type = CONFIG_ATOM_BOOLEAN;
        atom->atom_bool = true;
        break;
    case JSON_FALSE:
        atom->atom_type = CONFIG_ATOM_BOOLEAN;
        atom->atom_bool = false;
        break;
    case JSON_REAL:
        atom->atom_type = CONFIG_ATOM_FLOAT;
        atom->atom_float = json_real_value(json);
        break;
    case JSON_NULL:
        atom->atom_type = CONFIG_ATOM_NULL;
        break;
    case JSON_OBJECT:
        atom->atom_type = CONFIG_ATOM_NESTED;
        atom->atom_nested = json;
        break;
    }

    return ret;
}

aresult_t config_new(struct config **pcfg)
{
    aresult_t ret = A_OK;
    struct config *cfg = NULL;

    TSL_ASSERT_ARG(NULL != pcfg);

    *pcfg = NULL;

    cfg = calloc(1, sizeof(*cfg));

    if (NULL == cfg) {
        ret = A_E_NOMEM;
        goto done;
    }

    /* Create an empty JSON object to store the configuration */
    cfg->atom_type = CONFIG_ATOM_NESTED;
    cfg->atom_nested = json_object();

    if (NULL == cfg->atom_nested) {
        ret = A_E_NOMEM;
        goto done;
    }

    *pcfg = cfg;

done:
    if (AFAILED(ret)) {
        if (NULL != cfg) {
            if (NULL != cfg->atom_nested) {
                json_decref((json_t *)cfg->atom_nested);
                cfg->atom_nested = NULL;
            }
            free(cfg);
            cfg = NULL;
        }
    }
    return ret;
}

aresult_t config_add(struct config *cfg, const char *filename)
{
    aresult_t ret = A_OK;
    json_t *file_json = NULL;
    json_error_t err;

    TSL_ASSERT_ARG(NULL != cfg);
    TSL_ASSERT_ARG(NULL != filename);
    TSL_ASSERT_ARG('\0' != *filename);

    if (cfg->atom_type != CONFIG_ATOM_NESTED) {
        ret = A_E_INVAL;
        goto done;
    }

    TSL_ASSERT_ARG(NULL != cfg->atom_nested);

    file_json = json_load_file(filename, JSON_REJECT_DUPLICATES, &err);

    if (NULL == file_json) {
        CONFIG_MSG(SEV_FATAL, "Parse", "Error during JSON load & parse: %s (at line %d, source %s)", err.text, err.line, err.source);
        ret = A_E_INVAL;
        goto done;
    }

    if (0 > json_object_update((json_t *)cfg->atom_nested, file_json)) {
        DIAG("Error merging in file '%s' to configuration", filename);
        ret = A_E_INVAL;
        goto done;
    }

done:
    if (NULL != file_json) {
        json_decref(file_json);
        file_json = NULL;
    }

    return ret;
}

aresult_t config_get(struct config *cfg, struct config *atm, const char *item_id)
{
    aresult_t ret = A_OK;
    char *path = NULL;
    char *current_pos = NULL;
    char *next_pos = NULL;
    json_t *item = NULL;

    TSL_ASSERT_ARG(NULL != cfg);
    TSL_ASSERT_ARG(NULL != atm);
    TSL_ASSERT_ARG(NULL != item_id);
    TSL_ASSERT_ARG('\0' != *item_id);

    path = strdup(item_id);
    item = (json_t *)cfg->atom_nested;
    current_pos = path;

    do {
        next_pos = strchr(current_pos, '.');
        if (next_pos != NULL) {
            *next_pos++ = '\0';
        } else {
            next_pos = current_pos + strlen(current_pos);
        }
        if (next_pos == current_pos) {
            break;
        }

        item = json_object_get(item, current_pos);

        if (item == NULL) {
            ret = A_E_NOTFOUND;
            goto done;
        }
        current_pos = next_pos;
    } while (1);

    ret = __config_from_json(atm, item);

done:
    if (NULL != path) {
        free(path);
        path = NULL;
    }

    return ret;
}

aresult_t config_serialize(struct config *cfg, char **config)
{
    aresult_t ret = A_OK;
    char *ser = NULL;

    TSL_ASSERT_ARG(NULL != cfg);
    TSL_ASSERT_ARG(NULL != config);

    *config = NULL;

    if (NULL == (ser = json_dumps((json_t *)cfg->atom_nested, JSON_INDENT(2)))) {
        DIAG("Failed to serialize configuration to JSON, aborting.");
        ret = A_E_NOMEM;
        goto done;
    }

    *config = ser;

done:
    return ret;
}

aresult_t config_delete(struct config **pcfg)
{
    aresult_t ret = A_OK;
    struct config *cfg = NULL;

    TSL_ASSERT_ARG(NULL != pcfg);
    TSL_ASSERT_ARG(NULL != *pcfg);

    cfg = *pcfg;

    TSL_ASSERT_ARG(CONFIG_ATOM_NESTED == cfg->atom_type);


    if (NULL != cfg->atom_nested) {
        json_decref((json_t *)cfg->atom_nested);
        cfg->atom_nested = NULL;
    }

    memset(cfg, 0, sizeof(*cfg));
    free(cfg);
    *pcfg = NULL;

    return ret;
}

aresult_t config_array_length(struct config *atm, size_t *length)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(NULL != atm);
    TSL_ASSERT_ARG(NULL != length);

    if (CONFIG_ATOM_ARRAY != atm->atom_type) {
        ret = A_E_INVAL;
        goto done;
    }

    if (!json_is_array((json_t *)atm->atom_array)) {
        ret = A_E_INVAL;
        goto done;
    }

    *length = json_array_size(atm->atom_array);

done:
    return ret;
}

aresult_t config_array_at(struct config *array, struct config *item, size_t index)
{
    aresult_t ret = A_OK;
    json_t *arr_item = NULL;

    TSL_ASSERT_ARG(NULL != array);
    TSL_ASSERT_ARG(NULL != item);

    if (CONFIG_ATOM_ARRAY != array->atom_type) {
        ret = A_E_INVAL;
        goto done;
    }

    if (!json_is_array((json_t *)array->atom_array)) {
        ret = A_E_INVAL;
        goto done;
    }

    if (json_array_size(array->atom_array) <= index) {
        ret = A_E_INVAL;
        goto done;
    }

    if (NULL == (arr_item = json_array_get(array->atom_array, index))) {
        ret = A_E_INVAL;
        goto done;
    }

    ret = __config_from_json(item, arr_item);
done:
    return ret;
}

aresult_t config_array_at_integer(struct config *array, int *item, size_t index)
{
    aresult_t ret = A_OK;
    struct config atm;

    TSL_ASSERT_ARG(NULL != array);
    TSL_ASSERT_ARG(NULL != item);

    *item = 0;

    if (AFAILED(ret = config_array_at(array, &atm, index))) {
        goto done;
    }

    if (atm.atom_type != CONFIG_ATOM_INTEGER) {
        goto done;
    }

    *item = atm.atom_integer;

done:
    return ret;
}

aresult_t config_array_at_string(struct config *array, char **item, size_t index)
{
    aresult_t ret = A_OK;
    struct config atm;

    TSL_ASSERT_ARG(NULL != array);
    TSL_ASSERT_ARG(NULL != item);

    *item = NULL;

    if (AFAILED(ret = config_array_at(array, &atm, index))) {
        goto done;
    }

    if (atm.atom_type != CONFIG_ATOM_STRING) {
        goto done;
    }

    *item = atm.atom_string;

done:
    return ret;
}

aresult_t config_get_integer(struct config *cfg, int *val, const char *item_id)
{
    aresult_t ret = A_OK;
    struct config atm;

    TSL_ASSERT_ARG(NULL != cfg);
    TSL_ASSERT_ARG(NULL != val);

    if (AFAILED(ret = config_get(cfg, &atm, item_id))) {
        goto done;
    }

    if (CONFIG_ATOM_INTEGER != atm.atom_type) {
        goto done;
    }

    *val = atm.atom_integer;

done:
    return ret;
}

aresult_t config_get_boolean(struct config *cfg, bool *val, const char *item_id)
{
    aresult_t ret = A_OK;
    struct config atm;

    TSL_ASSERT_ARG(NULL != cfg);
    TSL_ASSERT_ARG(NULL != val);

    if (AFAILED(ret = config_get(cfg, &atm, item_id))) {
        goto done;
    }

    if (CONFIG_ATOM_BOOLEAN != atm.atom_type) {
        goto done;
    }

    *val = atm.atom_bool;

done:
    return ret;
}

aresult_t config_get_string(struct config *cfg, char **val, const char *item_id)
{
    aresult_t ret = A_OK;
    struct config atm;

    TSL_ASSERT_ARG(NULL != val);
    TSL_ASSERT_ARG(NULL != cfg);
    TSL_ASSERT_ARG(NULL != item_id);

    if (AFAILED(ret = config_get(cfg, &atm, item_id))) {
        goto done;
    }

    if (CONFIG_ATOM_STRING != atm.atom_type) {
        goto done;
    }

    *val = atm.atom_string;

done:
    return ret;
}

