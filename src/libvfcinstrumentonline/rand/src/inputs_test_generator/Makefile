all:
	verificarlo-c++ --std=c++17 --function=_Z8apply_opIdET_cS0_S0_ -DREAL=double test.cpp -o test_double
	verificarlo-c++ --std=c++17 --function=_Z8apply_opIfET_cS0_S0_ -DREAL=float test.cpp -o test_float

clean:
	rm -f test_double test_float .vfcwrapper* *.o