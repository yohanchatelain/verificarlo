lib_LTLIBRARIES = libvfcinstrumentonline.la librand.la

if ENABLE_WARNINGS
WARNING_FLAGS = -Wall -Wextra -Wno-varargs
else
WARNING_FLAGS =
endif

libvfcinstrumentonline_la_CXXFLAGS = @LLVM_CPPFLAGS@
libvfcinstrumentonline_la_LDFLAGS = @LLVM_LDFLAGS@
libvfcinstrumentonline_la_SOURCES = libVFCInstrumentOnline.cpp

librand_la_CXXFLAGS = @LLVM_CPPFLAGS@
librand_la_LDFLAGS = @LLVM_LDFLAGS@
librand_la_SOURCES = rand.c
