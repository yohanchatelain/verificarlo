ACLOCAL_AMFLAGS=-I m4

if ENABLE_RUST
# Build all Rust libraries using cargo workspace
all-local:
	cargo build --workspace --release

install-exec-local:
	$(MKDIR_P) $(DESTDIR)$(libdir)
	for lib in target/release/libinterflop_*.so; do \
		if [ -f $$lib ]; then \
			$(INSTALL) -m 0755 $$lib $(DESTDIR)$(libdir)/; \
		fi; \
	done
	for lib in target/release/libvfc_probes.a target/release/libvfcwrapper.a; do \
		if [ -f $$lib ]; then \
			$(INSTALL) -m 0644 $$lib $(DESTDIR)$(libdir)/; \
		fi; \
	done
	for component in hashmap fma rng; do \
		ln -sf libinterflop_stdlib.so $(DESTDIR)$(libdir)/libinterflop_$$component.so 2>/dev/null || true; \
	done
endif

all: $(SUBDIRS)

SUBDIRS=src/

dist_bin_SCRIPTS=verificarlo verificarlo-c verificarlo-f verificarlo-c++ verificarlo-rust

edit_script = $(SED) -e 's,%LIBDIR%,$(libdir),' -e 's,%INCLUDEDIR%,$(includedir),' $(NULL)

verificarlo: verificarlo.in Makefile
	$(AM_V_GEN)rm -f $@ $@.tmp && \
	$(edit_script) $< >$@.tmp && \
	chmod a-w $@.tmp && \
	mv $@.tmp $@

verificarlo-c:
	chmod a-w $@

verificarlo-c++:
	chmod a-w $@

verificarlo-f:
	chmod a-w $@

verificarlo-rust:
	chmod a-w $@

CLEANFILES = verificarlo

check:
	@echo "Tests should be run after install with make installcheck"

install-interflop-stdlib:
	make -C src/interflop-stdlib install

install-exec-hook: install-interflop-stdlib
	-$(PYTHON) -m pip install $(PYTHON_INSTALL_FLAGS) . || true
	@echo "Run 'source $(setenvdir)/set-env' to set environment variables"

installcheck:
# Run tests
# When successful print simple message
# If some tests have failed, report only the failing logs for the failed tests
	@if $(MAKE) -C tests/ check; then \
		echo "All tests have passed."; \
	else \
		grep FAIL tests/testplan.log | \
		while IFS= read -r line ; do \
			TEST=$$(echo $$line | cut -d' ' -f4); echo "************** TEST $${TEST}"; cat tests/$${TEST}/test.log; \
		done; \
		exit 1; \
	fi

# clean-local is a clean dependency of autotool clean target
clean-local: cleanstdlib cleantests clean-rust

clean-rust:
	cargo clean 2>/dev/null || true
	-rm -rf target

cleanstdlib:
# Clean interflop-stdlib directory
	cd src/interflop-stdlib && make clean && cd ../..

cleantests:
# Clean tests directory
	cd tests/ && ./clean.sh && cd ..
	@echo "tests directory is clean."
