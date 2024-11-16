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

ifndef operator
$(error operator is not set)
endif

ifdef size
vector_size=-DSIZE=$(size)
BINARY=test_$(type)_$(optimization)_$(operator)_$(size)
else
BINARY=test_$(type)_$(optimization)_$(operator)
endif

ifdef debug
DEBUG=--online-instrumentation-debug
else
DEBUG=
endif

ifdef loop
LOOP=-DN=$(loop)
else
LOOP=
endif

CC=verificarlo-c
SOURCE=test.c
CFLAGS=-g -Wall -DREAL_TYPE=$(REAL_TYPE) $(optimization) $(vector_size) -Wno-psabi $(DEBUG) z--online-instrumentation=sr --save-temps -march=native $(LOOP) # --show-cmd --verbose
LDFLAGS=--online-instrumentation=sr -lm -lstdc++

ifdef debug
CFLAGS+=--show-cmd --verbose
endif


all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) -L$(SRLIB)/src/ -lsr  -lhwy $(LDFLAGS)
