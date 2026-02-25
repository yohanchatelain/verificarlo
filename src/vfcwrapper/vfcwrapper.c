#include <stdlib.h>
#include <string.h>

#include "interflop/iostream/logger.h"
#include "vfcwrapper.h"

extern void vfc_atexit(const vfc_options_t *options);
extern void vfc_init(const vfc_options_t *options);

// Keep macro definitions for backward compatibility
// But using environment variable to set options at runtime seems more flexible?
static vfc_options_t _vfc_options = {
#ifdef INST_FCMP
    .inst_fcmp = 1,
#else
    .inst_fcmp = 0,
#endif
#ifdef DDEBUG
    .ddebug = 1,
#else
    .ddebug = 0,
#endif
#ifdef INST_FUNC
    .inst_func = 1,
#else
    .inst_func = 0,
#endif
#ifdef INST_FMA
    .inst_fma = 1,
#else
    .inst_fma = 0,
#endif
#ifdef INST_CAST
    .inst_cast = 1,
#else
    .inst_cast = 0
#endif
};

static void _exit_wrapper(void) { vfc_atexit(&_vfc_options); }

__attribute__((constructor(101))) static void _init(void) {

  char *vfc_options_env = getenv("VFC_INSTRUMENTATION_OPTIONS");
  if (vfc_options_env) {
    char *token = strtok(vfc_options_env, ",");
    while (token) {
      if (strcmp(token, "fcmp") == 0) {
        _vfc_options.inst_fcmp = 1;
      } else if (strcmp(token, "ddebug") == 0) {
        _vfc_options.ddebug = 1;
      } else if (strcmp(token, "function") == 0) {
        _vfc_options.inst_func = 1;
      } else if (strcmp(token, "fma") == 0) {
        _vfc_options.inst_fma = 1;
      } else if (strcmp(token, "cast") == 0) {
        _vfc_options.inst_cast = 1;
      } else {
        logger_warning("Unknown verificarlo instrumentation option: %s\n",
                       token);
      }
      token = strtok(NULL, ",");
    }
  }

  vfc_init(&_vfc_options);
  atexit(_exit_wrapper);
}