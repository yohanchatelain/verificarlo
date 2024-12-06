ifndef type
$(error type is not set)
endif

ifeq ($(type),double)
REAL_TYPE=1
else ifeq ($(type),float)
REAL_TYPE=2
else
$(error type must be either double or float)
endif

ifndef optimization
$(error optimization is not set)
endif


ifdef debug
DEBUG=--show-cmd --verbose --save-temps --online-instrumentation-debug
endif

ifdef size
vector_size=-DSIZE=$(size)
BINARY=test_$(type)_$(optimization)_$(operator)_$(size)
SOURCE=test_vector.c
INCLUDE=--include-file=include-functions.txt
else
BINARY=test_$(type)_$(optimization)_$(operator)
SOURCE=test.c
INCLUDE=
endif

CC=verificarlo-c
CFLAGS=-g -Wall -DREAL_TYPE=$(REAL_TYPE) $(optimization) $(vector_size) $(INCLUDE) -Wno-psabi -Wno-unknown-warning-option --online-instrumentation=up-down $(DEBUG) 
LDFLAGS=--online-instrumentation=up-down -lm 

ifdef static
CFLAGS+=--online-instrumentation-dispatch=static
LDFLAGS+=--online-instrumentation-dispatch=static
endif

ifdef native
CFLAGS+=-march=native
endif

all:
	@mkdir -p .objects .bin
	$(CC) $(CFLAGS) -c $(SOURCE) -o .objects/$(BINARY).o
	$(CC) .objects/$(BINARY).o -o .bin/$(BINARY) $(LDFLAGS)
