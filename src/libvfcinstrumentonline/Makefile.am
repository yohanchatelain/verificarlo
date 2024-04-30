lib_LTLIBRARIES = libvfcinstrumentonline.la

if ENABLE_WARNINGS
WARNING_FLAGS = -Wall -Wextra -Wno-varargs
else
WARNING_FLAGS =
endif

libvfcinstrumentonline_la_CXXFLAGS = @LLVM_CPPFLAGS@
libvfcinstrumentonline_la_LDFLAGS = @LLVM_LDFLAGS@
libvfcinstrumentonline_la_SOURCES = libVFCInstrumentOnline.cpp