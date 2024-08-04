load("//tests:macros.bzl", "cc_test_gen", "cc_test_lib_gen")

cc_test_gen(name = "get_exponent_test")

cc_test_gen(name = "get_predabs_test")

cc_test_gen(name = "get_pow2_test")

cc_test_gen(name = "twosum_test")

cc_test_gen(name = "twoprodfma_test")

cc_test_lib_gen(
    name = "udround_test",
    rng = "XOROSHIRO",
)

cc_test_lib_gen(
    name = "srround_test",
    size = "medium",
    rng = "XOROSHIRO",
)

test_suite(
    name = "all",
    tests = [
        ":get_exponent_test",
        ":get_pow2_test",
        ":get_predabs_test",
        ":srround_test",
        ":twoprodfma_test",
        ":twosum_test",
        ":udround_test",
    ],
)
