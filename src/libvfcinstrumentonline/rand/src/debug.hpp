#ifndef __VERIFICARLO_SRLIB_DEBUG_HPP__
#define __VERIFICARLO_SRLIB_DEBUG_HPP__

#include <cassert>

extern void __debug_printf(const char *fmt, ...);
extern void __debug_header_start(const char *func);
extern void __debug_header_end(const char *func);

#ifdef DEBUG
#ifndef DEBUG_FUNCTIONS_DECLARED
#include <cstdio>
#define __buffer_size 1024
static char __end = '\0';
static char __indent = '\t';
static int __debug_level = 0;
static char __debug_buffer[__buffer_size] = {__indent};

void __debug_printf(const char *fmt, ...) {
  assert(__debug_level >= 0);
  assert(__debug_level < __buffer_size);
  __debug_buffer[__debug_level] = __end;
  fprintf(stderr, "[debug] %s", __debug_buffer);
  __debug_buffer[__debug_level] = __indent;
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void __debug_header_start(const char *func) {
  assert(__debug_level >= 0);
  assert(__debug_level < __buffer_size);
  __debug_buffer[__debug_level] = __end;
  fprintf(stderr, "[debug] %s===%s===\n", __debug_buffer, func);
  __debug_buffer[__debug_level] = __indent;
  __debug_level++;
}

void __debug_header_end(const char *func) {
  __debug_level--;
  assert(__debug_level >= 0);
  assert(__debug_level < __buffer_size);
  __debug_buffer[__debug_level] = __end;
  fprintf(stderr, "[debug] %s===%s===\n\n", __debug_buffer, func);
  __debug_buffer[__debug_level] = __indent;
}
#define DEBUG_FUNCTIONS_DECLARED
#endif

#define debug_print(fmt, ...) __debug_printf(fmt, __VA_ARGS__)
#define debug_start() __debug_header_start(__func__)
#define debug_end() __debug_header_end(__func__)

#undef DEBUG
#else
#define debug_print(fmt, ...)
#define debug_start()
#define debug_end()
#endif

#endif // __VERIFICARLO_SRLIB_DEBUG_HPP__