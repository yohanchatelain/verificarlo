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

ifdef size
vector_size=-DSIZE=$(size)
BINARY=test_$(type)_$(optimization)_$(operator)_$(size)
SOURCE=test_vector.c
INCLUDE= 
else
BINARY=test_$(type)_$(optimization)_$(operator)
SOURCE=test.c
INCLUDE=--include-file=include-functions.txt
endif

CC=verificarlo-c
CFLAGS=-g -Wall -DREAL_TYPE=$(REAL_TYPE) $(optimization) $(vector_size) $(INCLUDE) -Wno-psabi --online-instrumentation=up-down # --show-cmd --verbose
LDFLAGS=--online-instrumentation=up-down -lm

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) $(LDFLAGS)
