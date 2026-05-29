if BUILD_PRISM
PRISM_SUBDIR = prism
else
PRISM_SUBDIR =
endif

SUBDIRS = @INTERFLOP_BACKENDS@ $(PRISM_SUBDIR)

if BUILD_PRISM
INTERFLOP_PRISM_SRC = $(srcdir)/interflop-backend-prism/interflop_prism.c
INTERFLOP_PRISM_LIB = libinterflop_prism.so

# libinterflop_prism.so is built at install time (not make time) because
# libprism-dynamic.so is only installed to $(INTERFLOP_LIBDIR) by the prism
# subdir's install-exec-hook, which runs before this install-exec-local.
install-exec-local:
	$(CC) -shared -fPIC -O2 \
	    -I$(INTERFLOP_INCLUDEDIR) \
	    -L$(INTERFLOP_LIBDIR) \
	    $(INTERFLOP_PRISM_SRC) \
	    -lprism-dynamic -linterflop_stdlib -linterflop_logger \
	    -Wl,-rpath,$(INTERFLOP_LIBDIR) \
	    -o $(INTERFLOP_PRISM_LIB)
	$(INSTALL) -m 0755 $(INTERFLOP_PRISM_LIB) $(DESTDIR)$(INTERFLOP_LIBDIR)

clean-local:
	rm -f $(INTERFLOP_PRISM_LIB)
endif