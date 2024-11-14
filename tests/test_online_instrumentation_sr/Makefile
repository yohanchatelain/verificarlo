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
SOURCE=test_vector.c
else
BINARY=test_$(type)_$(optimization)_$(operator)
SOURCE=test.c
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
CFLAGS=-g -Wall -DREAL_TYPE=$(REAL_TYPE) $(optimization) $(vector_size) -Wno-psabi $(DEBUG) --online-instrumentation=sr --save-temps -march=native $(LOOP) # --show-cmd --verbose
SRLIB=/home/yohan/snap/alacritty/common/.cache/bazel/_bazel_yohan/ae88ab9e827214603d27a251ccdde8c0/execroot/_main/bazel-out/k8-fastbuild/bin/
LDFLAGS=--online-instrumentation=sr -lm -lstdc++


all:
	$(CC) $(CFLAGS) -c $(SOURCE)   -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) -L$(SRLIB)/src/ -L$(SRLIB)/external/_main~_repo_rules~hwy/ -lsr  -lhwy $(LDFLAGS)
