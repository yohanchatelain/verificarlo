PRISM_BACKEND ?= sr

CXX     = verificarlo-c++
CFLAGS  = -O2 --prism-backend=$(PRISM_BACKEND) --include-file=include-functions.txt --inst-fma
LDFLAGS = --prism-backend=$(PRISM_BACKEND)

test_accuracy: test_accuracy.cpp
	$(CXX) $(CFLAGS) -c test_accuracy.cpp -o test_accuracy.o
	$(CXX) test_accuracy.o -o test_accuracy $(LDFLAGS)

clean:
	rm -f test_accuracy test_accuracy.o
