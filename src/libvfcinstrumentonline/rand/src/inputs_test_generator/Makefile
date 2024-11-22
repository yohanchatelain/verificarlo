CXXFLAGS = -O0 --inst-fma -std=c++17 --include-file=include.txt --verbose -mfma

all:
	verificarlo-c++ $(CXXFLAGS) -DREAL=double test.cpp -o test_double
	verificarlo-c++ $(CXXFLAGS) -DREAL=float  test.cpp -o test_float 

clean:
	rm -f test_double test_float .vfcwrapper* *.o