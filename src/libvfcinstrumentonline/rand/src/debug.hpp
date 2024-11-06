#ifndef __VERIFICARLO_SRLIB_DEBUG_HPP__
#define __VERIFICARLO_SRLIB_DEBUG_HPP__

#include <cassert>

extern void __debug_printf(const char *fmt, ...);
extern void __debug_header_start(const char *func);
extern void __debug_header_end(const char *func);

#ifdef SR_DEBUG
#ifndef SR_DEBUG_FUNCTIONS_DECLARED
#include <cstdarg>
#include <cstdio>
#define __buffer_size 1024
static char __end = '\0';
static char __indent = '\t';
static int __debug_level = 0;
static char __debug_buffer[__buffer_size] = {__indent};

#define __buffer_size_str 1048576
static char __debug_buffer_str[__buffer_size_str] = {__end};
static int __debug_str_pos = 0;
#endif // SR_DEBUG_FUNCTIONS_DECLARED

// TODO: make bufferized version of debug_print to avoid multiple calls to
// fprintf

void __debug_printf(const char *fmt, ...) {
  assert(__debug_level >= 0);
  assert(__debug_level < __buffer_size);
  __debug_buffer[__debug_level] = __end;
#ifdef SR_DEBUG_BUFFERIZED
  __debug_str_pos += snprintf(__debug_buffer_str + __debug_str_pos,
                              __buffer_size_str, "[debug] %s", __debug_buffer);
#else
  fprintf(stderr, "[debug] %s", __debug_buffer);
#endif
  __debug_buffer[__debug_level] = __indent;
  va_list args;
  va_start(args, fmt);
#ifdef SR_DEBUG_BUFFERIZED
  __debug_str_pos += vsnprintf(__debug_buffer_str + __debug_str_pos,
                               __buffer_size_str, fmt, args);
#else
  vfprintf(stderr, fmt, args);
#endif
  va_end(args);
}

void __debug_header_start(const char *func) {
  assert(__debug_level >= 0);
  assert(__debug_level < __buffer_size);
  __debug_buffer[__debug_level] = __end;
#ifdef SR_DEBUG_BUFFERIZED
  __debug_str_pos +=
      snprintf(__debug_buffer_str + __debug_str_pos, __buffer_size_str,
               "[debug] %s===%s===\n", __debug_buffer, func);
#else
  fprintf(stderr, "[debug] %s===%s===\n", __debug_buffer, func);
#endif
  __debug_buffer[__debug_level] = __indent;
  __debug_level++;
}

void __debug_header_end(const char *func) {
  __debug_level--;
  assert(__debug_level >= 0);
  assert(__debug_level < __buffer_size);
  __debug_buffer[__debug_level] = __end;
#ifdef SR_DEBUG_BUFFERIZED
  __debug_str_pos +=
      snprintf(__debug_buffer_str + __debug_str_pos, __buffer_size_str,
               "[debug] %s===%s===\n\n", __debug_buffer, func);
#else
  fprintf(stderr, "[debug] %s===%s===\n\n", __debug_buffer, func);
#endif
  __debug_buffer[__debug_level] = __indent;
}

void __debug_reset() {
  __debug_str_pos = 0;
  __debug_buffer_str[__debug_str_pos] = __end;
}

void __debug_flush() {
  fprintf(stderr, "%s", __debug_buffer_str);
  __debug_reset();
}
#endif // SR_DEBUG

#define DEBUG_FUNCTIONS_DECLARED

#ifdef SR_DEBUG
#define debug_print(fmt, ...) __debug_printf(fmt, __VA_ARGS__)
#define debug_start() __debug_header_start(__func__)
#define debug_end() __debug_header_end(__func__)
#define debug_flush() fprintf(stderr, "%s", __debug_buffer_str)
#define debug_reset() __debug_reset()
#else
#define debug_print(fmt, ...)
#define debug_start()
#define debug_end()
#define debug_flush()
#define debug_reset()
#endif // SR_DEBUG

#endif // __VERIFICARLO_SRLIB_DEBUG_HPP__