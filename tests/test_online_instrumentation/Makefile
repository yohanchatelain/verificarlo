ifndef type
$(error type is not set)
endif

ifndef optimization
$(error optimization is not set)
endif


CC=verificarlo-c
CFLAGS=-g -Wall -DREAL=$(type) $(optimization) --function=operator --online-instrumentation=up-down # --show-cmd --verbose
LDFLAGS=--online-instrumentation=up-down
SOURCE=test.c
BINARY=test_$(type)_$(optimization)

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) $(LDFLAGS)
