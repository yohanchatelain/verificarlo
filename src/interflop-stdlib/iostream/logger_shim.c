#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "logger.h"

__attribute__((visibility("default")))
void interflop_stdlib_logger_anchor(void) {}

static const char *backend_name = "verificarlo";

__attribute__((visibility("default")))
void logger_init(interflop_panic_t panic, File *stream, const char *backend_header_name) {
    (void)panic;
    (void)stream;
    if (backend_header_name != NULL) {
        backend_name = backend_header_name;
    }
}

static int is_enabled(void) {
    const char *env = getenv("VFC_BACKENDS_LOGGER");
    if (env == NULL || strcasecmp(env, "True") == 0) return 1;
    return 0;
}

static int is_debug_enabled(void) {
    if (!is_enabled()) return 0;
    const char *lvl = getenv("VFC_BACKENDS_LOGGER_LEVEL");
    if (lvl != NULL && strcasecmp(lvl, "debug") == 0) return 1;
    return 0;
}

__attribute__((visibility("default")))
void logger_info(const char *fmt, ...) {
    if (!is_enabled() || stderr == NULL || fmt == NULL) return;
    fprintf(stderr, "Info [%s]: ", backend_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

__attribute__((visibility("default")))
void logger_warning(const char *fmt, ...) {
    if (!is_enabled() || stderr == NULL || fmt == NULL) return;
    fprintf(stderr, "Warning [%s]: ", backend_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

__attribute__((visibility("default")))
void logger_error(const char *fmt, ...) {
    if (stderr == NULL || fmt == NULL) return;
    fprintf(stderr, "Error [%s]: ", backend_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

__attribute__((visibility("default")))
void logger_debug(const char *fmt, ...) {
    if (!is_debug_enabled() || stderr == NULL || fmt == NULL) return;
    fprintf(stderr, "Debug [%s]: ", backend_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}
