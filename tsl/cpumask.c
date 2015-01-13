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
#include <tsl/cpumask.h>
#include <tsl/assert.h>
#include <tsl/errors.h>
#include <tsl/diag.h>

#include <tsl/config/engine.h>

#include <stdlib.h>

#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Abstraction on top of Linux CPU thread masks */
struct cpu_mask {
    cpu_set_t *mask;
    long num_cpus;
    long size;
};

aresult_t cpu_mask_from_config(struct cpu_mask **pmask, struct config *cfg, const char *field_name)
{
    aresult_t ret = A_OK;

    struct cpu_mask *msk = NULL;
    int core_id = -1;
    struct config core_arr;

    TSL_ASSERT_ARG(NULL != pmask);
    TSL_ASSERT_ARG(NULL != cfg);
    TSL_ASSERT_ARG(NULL != field_name);

    if (AFAILED(ret = cpu_mask_create(&msk))) {
        goto done;
    }

    if (!AFAILED(ret = config_get_integer(cfg, &core_id, field_name))) {
        if (0 > core_id) {
            DIAG("Negative core ID specified, aborting.");
            goto done;
        }

        if (AFAILED(ret = cpu_mask_set(msk, core_id))) {
            DIAG("Failed to set CPU Core mask: %d", core_id);
            goto done;
        }

    } else if (!AFAILED(ret = config_get(cfg, &core_arr, field_name))) {
        size_t nr_entries = 0;
        bool failed = false;
        size_t num_set = 0;

        if (AFAILED(ret = config_array_length(&core_arr, &nr_entries))) {
            DIAG("Array is malformed.");
            goto done;
        }

        if (0 == nr_entries) {
            DIAG("Array is empty, need to specify an array of CPU core ID integers.");
            goto done;
        }

        for (size_t i = 0; i < nr_entries; i++) {
            int core_id = -1;
            if (AFAILED(ret = config_array_at_integer(&core_arr, &core_id, i))) {
                DIAG("Array entry %zu is not an integer, skipping.", i);
                failed = true;
                continue;
            }

            if (0 > core_id) {
                DIAG("Core ID at %zu is invalid (%d is less than 0)", i, core_id);
                failed = true;
                continue;
            }

            if (AFAILED(ret = cpu_mask_set(msk, core_id))) {
                DIAG("Invalid core ID specified: %d at offset %zu", core_id, i);
                failed = true;
                continue;
            }

            num_set++;
        }

        if (true == failed || 0 == num_set) {
            DIAG("Failed to populate CPU core, malformed array entries were found.");
            ret = A_E_INVAL;
            goto done;
        }
    } else {
        DIAG("Failed to find CPU core configuration field '%s'", field_name);
        ret = A_E_NOENT;
        goto done;
    }

done:
    if (AFAILED(ret)) {
        if (NULL != msk) {
            cpu_mask_destroy(&msk);
        }
    } else {
        *pmask = msk;
    }

    return ret;
}

aresult_t cpu_mask_create(struct cpu_mask **mask)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(mask != NULL);
    *mask = NULL;

    struct cpu_mask *msk = (struct cpu_mask *)calloc(1, sizeof(struct cpu_mask));

    if (msk == NULL) {
        ret = A_E_NOMEM;
        goto done;
    }

    msk->num_cpus = sysconf(_SC_NPROCESSORS_CONF);

    DIAG("Creating a CPU Set Mask for %ld CPUs.", msk->num_cpus);

    msk->mask = CPU_ALLOC(msk->num_cpus);

    if (!msk->mask) {
        ret = A_E_NOMEM;
        goto done_fail;
    }

    msk->size = CPU_ALLOC_SIZE(msk->num_cpus);
    CPU_ZERO_S(msk->size, msk->mask);

    *mask = msk;

done_fail:
    if (AFAILED(ret)) {
        if (msk) {
            if (msk->mask) {
                CPU_FREE(msk->mask);
                msk->mask = NULL;
            }
            free(msk);
        }
    }

done:
    return ret;
}

aresult_t cpu_mask_clear(struct cpu_mask *mask, size_t cpu_id)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(mask != NULL);

    CPU_CLR_S(cpu_id, mask->size, mask->mask);

    return ret;
}

aresult_t cpu_mask_set(struct cpu_mask *mask, size_t cpu_id)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(mask != NULL);

    CPU_SET_S(cpu_id, mask->size, mask->mask);

    return ret;
}

aresult_t cpu_mask_clear_all(struct cpu_mask *mask)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(mask != NULL);

    CPU_ZERO_S(mask->size, mask->mask);

    return ret;

}

aresult_t cpu_mask_set_all(struct cpu_mask *mask)
{
    aresult_t ret = A_E_INVAL;

    TSL_ASSERT_ARG(mask != NULL);

    return ret;

}

aresult_t cpu_mask_clone(struct cpu_mask **_new,
                         struct cpu_mask *orig)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(_new != NULL);
    *_new = NULL;
    TSL_ASSERT_ARG(orig != NULL);


    return ret;
}

aresult_t cpu_mask_destroy(struct cpu_mask **mask)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(mask != NULL);
    TSL_ASSERT_ARG(*mask != NULL);

    struct cpu_mask *msk = *mask;

    if (msk->mask) {
        CPU_FREE(msk->mask);
    }
    free(msk);

    *mask = NULL;

    return ret;

}

aresult_t cpu_mask_test(struct cpu_mask *mask,
                        size_t cpu_id,
                        int *value)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(mask != NULL);
    TSL_ASSERT_ARG(value != NULL);

    *value = CPU_ISSET_S(cpu_id, mask->size, mask->mask);

    return ret;
}

aresult_t cpu_mask_apply(struct cpu_mask *mask)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(mask);

    if (sched_setaffinity(0, mask->size, mask->mask) != 0) {
        DIAG("sched_setaffinity: failure. %d - %s", errno, strerror(errno));
        ret = A_E_INVAL;
        goto done;
    }

done:
    return ret;
}

