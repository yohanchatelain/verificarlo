PRISM_BACKEND ?= sr

CXX = verificarlo-c

CFLAGS  = -O2 --prism-backend=$(PRISM_BACKEND)
LDFLAGS = --prism-backend=$(PRISM_BACKEND)

test: test.c
	$(CXX) $(CFLAGS) -c test.c -o test.o
	$(CXX) test.o -o test $(LDFLAGS)

clean:
	rm -f test test.o
