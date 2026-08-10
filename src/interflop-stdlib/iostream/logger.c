/*****************************************************************************\
 *                                                                           *\
 *  This File is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2024                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "logger.h"

#if defined(__cplusplus)
extern "C" {
#endif

static const char *backend_header = NULL;

/* ANSI colors */
typedef enum {
  red = 0,
  green,
  yellow,
  blue,
  magenta,
  cyan,
  gray,
  bold_red,
  bold_green,
  bold_yellow,
  bold_blue,
  bold_magenta,
  bold_cyan,
  bold_gray,
  reset
} ansi_colors_t;

/* ANSI escape sequences for normal colors */
static const char ansi_color_red[] = "\x1b[31m";
static const char ansi_color_green[] = "\x1b[32m";
static const char ansi_color_yellow[] = "\x1b[33m";
static const char ansi_color_blue[] = "\x1b[34m";
static const char ansi_color_magenta[] = "\x1b[35m";
static const char ansi_color_cyan[] = "\x1b[36m";
static const char ansi_color_gray[] = "\x1b[38;5;250m";
static const char ansi_color_reset[] = "\x1b[0m";

/* ANSI escape sequences for bold colors */
static const char ansi_color_bold_red[] = "\x1b[1;31m";
static const char ansi_color_bold_green[] = "\x1b[1;32m";
static const char ansi_color_bold_yellow[] = "\x1b[1;33m";
static const char ansi_color_bold_blue[] = "\x1b[1;34m";
static const char ansi_color_bold_magenta[] = "\x1b[1;35m";
static const char ansi_color_bold_cyan[] = "\x1b[1;36m";
static const char ansi_color_bold_gray[] = "\x1b[1;38;5;250m";

/* Array of ANSI colors */
static const char *ansi_colors[] = {
    [red] = ansi_color_red,
    [green] = ansi_color_green,
    [yellow] = ansi_color_yellow,
    [blue] = ansi_color_blue,
    [magenta] = ansi_color_magenta,
    [cyan] = ansi_color_cyan,
    [gray] = ansi_color_gray,
    [bold_red] = ansi_color_bold_red,
    [bold_green] = ansi_color_bold_green,
    [bold_yellow] = ansi_color_bold_yellow,
    [bold_blue] = ansi_color_bold_blue,
    [bold_magenta] = ansi_color_bold_magenta,
    [bold_cyan] = ansi_color_bold_cyan,
    [bold_gray] = ansi_color_bold_gray,
    [reset] = ansi_color_reset,
};

typedef enum {
  backend_color = green,
  debug_color = bold_gray,
  info_color = bold_blue,
  warning_color = bold_yellow,
  error_color = bold_red,
  reset_color = reset
} level_color;

typedef enum {
  logger_level_debug = 0,
  logger_level_info,
  logger_level_warning,
  logger_level_error
} logger_level_t;

static const char vfc_backends_logger[] = "VFC_BACKENDS_LOGGER";
static const char vfc_backends_logger_level[] = "VFC_BACKENDS_LOGGER_LEVEL";
static const char vfc_backends_logfile[] = "VFC_BACKENDS_LOGFILE";
static const char vfc_backends_colored_logger[] = "VFC_BACKENDS_COLORED_LOGGER";

static FILE *logger_logfile = NULL;
static FILE *logger_stderr = NULL;
static int logger_level = logger_level_info;

IBool is_logger_enabled(void) {
  const char *is_logger_enabled_env = getenv(vfc_backends_logger);
  if (is_logger_enabled_env == NULL) {
    return ITrue;
  } else if (strcasecmp(is_logger_enabled_env, "True") == 0) {
    return ITrue;
  } else {
    return IFalse;
  }
}

IBool is_logger_colored(void) {
  const char *is_colored_logger_env = getenv(vfc_backends_colored_logger);
  if (is_colored_logger_env == NULL) {
    return IFalse;
  } else if (strcasecmp(is_colored_logger_env, "True") == 0) {
    return ITrue;
  } else {
    return IFalse;
  }
}

logger_level_t get_logger_level(void) {
  const char *logger_level_env = getenv(vfc_backends_logger_level);
  if (logger_level_env == NULL) {
    return logger_level_info;
  } else if (strcasecmp(logger_level_env, "debug") == 0) {
    return logger_level_debug;
  } else if (strcasecmp(logger_level_env, "info") == 0) {
    return logger_level_info;
  } else if (strcasecmp(logger_level_env, "warning") == 0) {
    return logger_level_warning;
  } else if (strcasecmp(logger_level_env, "error") == 0) {
    return logger_level_error;
  } else {
    return logger_level_info;
  }
}

void set_logger_logfile() {
  if (logger_logfile != NULL) {
    return;
  }
  const char *logger_logfile_env = getenv(vfc_backends_logfile);
  if (logger_logfile_env == NULL) {
    logger_logfile = logger_stderr ? logger_stderr : stderr;
  } else {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s.%d", logger_logfile_env, getpid());
    logger_logfile = fopen(tmp, "a");
    if (logger_logfile == NULL) {
      logger_logfile = stderr;
    }
  }
}

static void logger_header(FILE *stream, const char *lvl_name,
                          const level_color lvl_color, const IBool colored) {
  if (stream == NULL) return;
  const char *header = backend_header ? backend_header : "verificarlo";
  if (colored) {
    fprintf(stream, "%s%s%s [%s%s%s]: ", ansi_colors[lvl_color],
            lvl_name, ansi_colors[reset_color],
            ansi_colors[backend_color], header,
            ansi_colors[reset_color]);
  } else {
    fprintf(stream, "%s [%s]: ", lvl_name, header);
  }
}

void logger_debug(const char *fmt, ...) {
  if (is_logger_enabled() && logger_level <= logger_level_debug) {
    FILE *stream = logger_logfile ? logger_logfile : stderr;
    if (stream == NULL) return;
    logger_header(stream, "Debug", debug_color, is_logger_colored());
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stream, fmt, ap);
    va_end(ap);
  }
}

void logger_info(const char *fmt, ...) {
  if (is_logger_enabled() && logger_level <= logger_level_info) {
    FILE *stream = logger_logfile ? logger_logfile : stderr;
    if (stream == NULL) return;
    logger_header(stream, "Info", info_color, is_logger_colored());
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stream, fmt, ap);
    va_end(ap);
  }
}

void logger_warning(const char *fmt, ...) {
  FILE *stream = logger_stderr ? logger_stderr : stderr;
  if (stream == NULL) return;
  if (is_logger_enabled() && logger_level <= logger_level_warning) {
    logger_header(stream, "Warning", warning_color, is_logger_colored());
  }
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stream, fmt, ap);
  va_end(ap);
}

void logger_error(const char *fmt, ...) {
  FILE *stream = logger_stderr ? logger_stderr : stderr;
  if (stream == NULL) return;
  if (is_logger_enabled() && logger_level <= logger_level_error) {
    logger_header(stream, "Error", error_color, is_logger_colored());
  }
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stream, fmt, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

void vlogger_debug(const char *fmt, va_list argp) {
  if (is_logger_enabled() && logger_level <= logger_level_debug) {
    FILE *stream = logger_logfile ? logger_logfile : stderr;
    if (stream == NULL) return;
    logger_header(stream, "Debug", debug_color, is_logger_colored());
    vfprintf(stream, fmt, argp);
  }
}

void vlogger_info(const char *fmt, va_list argp) {
  if (is_logger_enabled() && logger_level <= logger_level_info) {
    FILE *stream = logger_logfile ? logger_logfile : stderr;
    if (stream == NULL) return;
    logger_header(stream, "Info", info_color, is_logger_colored());
    vfprintf(stream, fmt, argp);
  }
}

void vlogger_warning(const char *fmt, va_list argp) {
  FILE *stream = logger_stderr ? logger_stderr : stderr;
  if (stream == NULL) return;
  if (is_logger_enabled() && logger_level <= logger_level_warning) {
    logger_header(stream, "Warning", warning_color, is_logger_colored());
  }
  vfprintf(stream, fmt, argp);
}

void vlogger_error(const char *fmt, va_list argp) {
  FILE *stream = logger_stderr ? logger_stderr : stderr;
  if (stream == NULL) return;
  if (is_logger_enabled() && logger_level <= logger_level_error) {
    logger_header(stream, "Error", error_color, is_logger_colored());
  }
  vfprintf(stream, fmt, argp);
  exit(EXIT_FAILURE);
}

void logger_init(interflop_panic_t panic, File *stream,
                 const char *backend_header_name) {
  (void)panic;
  backend_header = backend_header_name;
  logger_stderr = stream ? (FILE*)stream : stderr;
  logger_level = get_logger_level();
  set_logger_logfile();
}

#if defined(__cplusplus)
}
#endif