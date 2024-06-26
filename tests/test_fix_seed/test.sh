#!/bin/bash

Check_status() {
	if [[ $? != 0 ]]; then
		echo "error: test failed"
		exit 1
	fi
}

Check_difference() {
	ret=$(diff -q $1 $2)
	if [[ $ret == 0 ]]; then
		echo "error: files must be different"
		exit 1
	else
		echo
	fi
}

Check_similarity() {
	diff -s $1 $2
	if [[ $? == 1 ]]; then
		echo "error: files must be the same"
		exit 1
	fi
}

Check() {
	PREC=$1
	MODE="MCA"

	for BACKEND in "mca"; do
		echo -e "\nChecking backend ${BACKEND} at PRECISION $PREC MODE $MODE"

		BACKENDSO="libinterflop_${BACKEND}.so"

		out_seed_1="out_seed_${BACKEND}.1"
		out_seed_2="out_seed_${BACKEND}.2"

		out_no_seed_1="out_no_seed_${BACKEND}.1"
		out_no_seed_2="out_no_seed_${BACKEND}.2"

		rm -f out_seed_{1,2} out_no_seed_{1,2}

		export VFC_BACKENDS="${BACKENDSO} --precision-binary64=$PREC --mode $MODE"
		./test 2>$out_no_seed_1
		./test 2>$out_no_seed_2
		Check_difference $out_no_seed_1 $out_no_seed_2

		export VFC_BACKENDS="${BACKENDSO} --precision-binary64=$PREC --mode $MODE --seed=0"
		./test 2>$out_seed_1
		./test 2>$out_seed_2
		Check_similarity $out_seed_1 $out_seed_2

	done
}

echo "Checking"
verificarlo-c -O0 test.c -o test
Check_status
Check 53

echo -e "\nsuccess"
