ifndef type
$(error type is not set)
endif

ifndef optimization
$(error optimization is not set)
endif

ifndef operator
$(error operator is not set)
endif

CC=verificarlo-c
CFLAGS=-g -Wall -DREAL=$(type) $(optimization) --online-instrumentation=sr --save-temps # --show-cmd --verbose
LDFLAGS=--online-instrumentation=sr -lm
SOURCE=test.c
BINARY=test_$(type)_$(optimization)_$(operator)

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) $(LDFLAGS)
