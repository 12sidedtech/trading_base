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
#ifndef __INCLUDED_TSL_APP_H__
#define __INCLUDED_TSL_APP_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <tsl/sections.h>
#include <tsl/result.h>

/**
 * Function pointer for function to be called to initialize an app subsystem.
 */
typedef aresult_t (*app_subsystem_init_func_t)(void);

/**
 * Function pointer for a function to be called to shut down an app subsystem.
 */
typedef aresult_t (*app_subsystem_shutdown_func_t)(void);

/**
 * Declaration of a dynamic compilation application subsystem.
 */
struct app_subsystem {
    const char *name;
    app_subsystem_init_func_t init;
    app_subsystem_shutdown_func_t shutdown;
};

/**
 * Declare a new app subsystem and make it available at run-time
 */
#define APP_SUBSYSTEM(__name, __init_func, __shutdown_func) \
    static struct app_subsystem __name ## _subsystem_decl = {           \
        .name = #__name,                                                \
        .init = (__init_func),                                          \
        .shutdown = (__shutdown_func)                                   \
    };                                                                  \
    CR_LOADABLE(__dynamic_subsystems, __name ## _subsystem_decl);

/**
 * Initialize all dynamic subsystems of an application
 */
aresult_t app_init(const char *app_name);

typedef aresult_t (*app_sigint_handler_t)(void);

/**
 * Attach a SIGINT handler so shutdown can be done gracefully
 * \param delegate Function that is delegated to on SIGINT's arrival
 * \return A_OK on success, an error code otherwise
 * \note Use app_running() predicate function to check if a halt has been signaled.
 */
aresult_t app_sigint_catch(app_sigint_handler_t hdlr);

/**
 * Predicate to check if a shutdown has been requested
 * \return 1 on app is to be running, 0 when app is to shutdown
 */
int app_running(void);

/**
 * Get the name (string) of the running application
 */
aresult_t app_get_name(const char **app_name);

aresult_t app_daemonize(void);
aresult_t app_set_diag_output(const char *file_name);
aresult_t app_bind_cpu_core(int core_id);

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_TSL_APP_H__ */

