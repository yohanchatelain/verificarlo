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
    name = "srround_boost_test",
    size = "medium",
    src = "//tests:srround_test.cpp",
    rng = "CXX11_RANDOM",
)

cc_test_lib_gen(
    name = "srround_xoroshiro_test",
    size = "medium",
    src = "//tests:srround_test.cpp",
    rng = "XOROSHIRO",
)

cc_test_lib_gen(
    name = "srround_xoroshiro_test_x2",
    size = "medium",
    src = "//tests:srround_test_x2.cpp",
    rng = "XOROSHIRO",
)

cc_test_lib_gen(
    name = "srround_shishua_test",
    size = "medium",
    src = "//tests:srround_test.cpp",
    rng = "SHISHUA",
)

cc_test_lib_gen(
    name = "srround_xoroshiro_test_hw",
    size = "medium",
    src = "//tests:srround_test_hw.cpp",
    rng = "XOROSHIRO",
    deps = [
        "@hwy",
        "@hwy//:math",
    ],
)

test_suite(
    name = "all",
    tests = [
        ":get_exponent_test",
        ":get_pow2_test",
        ":get_predabs_test",
        ":srround_boost_test",
        ":srround_shishua_test",
        ":srround_xoroshiro_test",
        ":twoprodfma_test",
        ":twosum_test",
        ":udround_test",
    ],
)
