/*
  Copyright (c) 2014 12Sided Technology LLC.
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
#include <tsl/app.h>
#include <tsl/diag.h>
#include <tsl/app.h>
#include <tsl/panic.h>
#include <tsl/assert.h>
#include <tsl/cpumask.h>

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <execinfo.h>

#define APP_STATE_RUNNING               0
#define APP_STATE_SHUTDOWN_REQUESTED    1
#define APP_STATE_SHUTDOWN_FORCED       2

static volatile
int __app_state = APP_STATE_RUNNING;

static
const char *app_name_str = NULL;

static
app_sigint_handler_t __app_sigint_handler = NULL;

/**
 * Dump the backtrace for diagnostic purposes.
 */
static
void segv_handler(int signal)
{
    void *symbols[20];
    size_t len = 0;
    len = backtrace(symbols, BL_ARRAY_ENTRIES(symbols));
    printf("\nsegmentation fault - backtracing %zu frames.\n", len);
    backtrace_symbols_fd(symbols, len, STDERR_FILENO);
    printf("aborting.\n");
    abort();
}

static
void app_sigint_handler(int signal)
{
    __app_state++;

    DIAG("Interrupt signal received.");

    if (__app_state >= APP_STATE_SHUTDOWN_FORCED) {
        PANIC("User insisted that application terminate. Aborting.");
    }

    if (NULL != __app_sigint_handler) {
        __app_sigint_handler();
    }
}

static
aresult_t _segv_handler_install(void)
{
    aresult_t ret = A_OK;
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = segv_handler;

    if (0 > sigaction(SIGSEGV, &sa, NULL)) {
        PDIAG("Failed to install SEGV handler.");
        ret = A_E_INVAL;
        goto done;
    }

done:
    return ret;
}

APP_SUBSYSTEM(sigsegv, _segv_handler_install, NULL);

int app_running(void)
{
    return (__app_state == APP_STATE_RUNNING);
}

aresult_t app_sigint_catch(app_sigint_handler_t hdlr)
{
    aresult_t ret = A_OK;
    struct sigaction sa;

    __app_sigint_handler = hdlr;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = app_sigint_handler;

    if (0 > sigaction(SIGINT, &sa, NULL)) {
        PDIAG("Failed to install SIGINT handler.");
        ret = A_E_INVAL;
        goto done;
    }

done:
    return ret;
}

/**
 * Daemonize the current application. Fork the application and terminate the parent
 * thread.
 * \return A_OK on success, an error code otherwise.
 */
aresult_t app_daemonize(void)
{
    aresult_t ret = A_OK;
    pid_t proc_id = 0;
    pid_t session_id = 0;

    proc_id = fork();

    if (proc_id < 0) {
        PDIAG("Unable to fork(2) process from parent.");
        ret = A_E_UNKNOWN;
        goto done;
    }

    if (proc_id > 0) {
        /* Terminate the invoking process */
        exit(EXIT_SUCCESS);
    }

    session_id = setsid();

    if (session_id < 0) {
        PDIAG("Failed to set session ID");
        ret = A_E_INVAL;
        goto done;
    }

    /* Set working directory to the root of the filesystem */
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

done:
    return ret;
}

/**
 * Bind app to a specified CPU core
 * \param core_id the CPU core to bind to
 * \return A_OK on success, an error code otherwise
 */
aresult_t app_bind_cpu_core(int core_id)
{
    aresult_t ret = A_OK;
    struct cpu_mask *msk = NULL;

    TSL_ASSERT_ARG(core_id >= 0);

    if (AFAILED(ret = cpu_mask_create(&msk))) {
        goto done;
    }

    if (AFAILED(ret = cpu_mask_set(msk, core_id))) {
        goto done;
    }

    if (AFAILED(ret = cpu_mask_apply(msk))) {
        goto done;
    }

    if (AFAILED(ret = cpu_mask_destroy(&msk))) {
        goto done;
    }

done:
    return ret;
}

/**
 * Redirect any diagnostic outputs to a file for later consumption.
 * \param file_name the name of the file to redirect to
 * \return A_OK on success, A_E_INVAL if file could not be opened.
 */
aresult_t app_set_diag_output(const char *file_name)
{
    aresult_t ret = A_OK;
    FILE *reopen = NULL;

    TSL_ASSERT_ARG(NULL != file_name);

    if (NULL == (reopen = freopen(file_name, "a+", stdout))) {
        PDIAG("Failed to redirect diag output. Aborting.");
        ret = A_E_INVAL;
        goto done;
    }

done:
    return ret;
}

aresult_t app_init(const char *app_name)
{
    struct app_subsystem *subsys = NULL;

    TSL_ASSERT_ARG(app_name != NULL);

    app_name_str = app_name;

    CR_FOR_EACH_LOADABLE(subsys, __dynamic_subsystems) {
        if (!subsys->init) {
            continue;
        }

        DIAG("Initializing '%s' subsystem...", subsys->name);
        if (AFAILED(subsys->init())) {
            PANIC("Failed to initialize subsystem '%s'", subsys->name);
        }
    }

    return A_OK;
}

APP_SUBSYSTEM(default_subsystem, NULL, NULL);
