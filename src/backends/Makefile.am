if BUILD_PRISM
PRISM_SUBDIR = prism
else
PRISM_SUBDIR =
endif

SUBDIRS = @INTERFLOP_BACKENDS@ $(PRISM_SUBDIR)

if BUILD_PRISM
INTERFLOP_PRISM_SRC = $(srcdir)/interflop-backend-prism/interflop_prism.c
INTERFLOP_PRISM_LIB = libinterflop_prism.so

$(INTERFLOP_PRISM_LIB): $(INTERFLOP_PRISM_SRC) prism
	$(CC) -shared -fPIC -O2 \
	    -I$(INTERFLOP_INCLUDEDIR) \
	    -L$(INTERFLOP_LIBDIR) \
	    $(INTERFLOP_PRISM_SRC) \
	    -lprism-dynamic -linterflop_stdlib -linterflop_logger \
	    -Wl,-rpath,$(INTERFLOP_LIBDIR) \
	    -o $@

all-local: $(INTERFLOP_PRISM_LIB)

install-exec-local: $(INTERFLOP_PRISM_LIB)
	$(INSTALL) -m 0755 $(INTERFLOP_PRISM_LIB) $(DESTDIR)$(INTERFLOP_LIBDIR)

clean-local:
	rm -f $(INTERFLOP_PRISM_LIB)
endif