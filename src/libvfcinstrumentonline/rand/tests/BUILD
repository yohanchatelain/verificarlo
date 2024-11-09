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
    src = ["//tests:srround_test.cpp"],
    rng = "CXX11_RANDOM",
)

cc_test_lib_gen(
    name = "srround_xoroshiro_test",
    size = "medium",
    src = ["//tests:srround_test.cpp"],
    rng = "XOROSHIRO",
)

cc_test_lib_gen(
    name = "srround_xoroshiro_test_x2",
    size = "medium",
    src = ["//tests:srround_test_x2.cpp"],
    rng = "XOROSHIRO",
)

cc_test_lib_gen(
    name = "srround_shishua_test",
    size = "medium",
    src = ["//tests:srround_test.cpp"],
    rng = "SHISHUA",
)

cc_test_lib_gen(
    name = "srround_xoroshiro_test_hw",
    size = "medium",
    src = ["//tests:srround_test_hw.cpp"],
    rng = "XOROSHIRO",
    deps = [
        "@hwy",
        "@hwy//:math",
    ],
)

cc_test_lib_gen(
    name = "xoroshiro",
    size = "medium",
    src = ["//tests:xoroshiro.cpp"],
    rng = "XOROSHIRO",
    copts = [
        "-O2",
        "-Wfatal-errors",
        "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)",
        "-DSR_DEBUG"
    ],
    deps = [
        "@hwy",
        "@hwy//:math",
        "@hwy//:hwy_test_util",
    ],
)



cc_test_lib_gen(
    name = "srround_xoroshiro_test_hw-inl",
    size = "medium",
    src = ["//tests:srround_test_hw-inl.cpp"],
    rng = "XOROSHIRO",
    copts = [
        "-O2",
        "-Wfatal-errors",
        "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)",
        "-DSR_DEBUG"
    ],
    deps = [
        "@hwy",
        "@hwy//:math",
        "@hwy//:hwy_test_util",
    ],
)


cc_test_lib_gen(
    name = "sr_hw_test",
    size = "medium",
    src = ["//tests:sr_hw_test.cpp"],
    rng = "XOROSHIRO",
    copts = [
        "-O2",
        "-march=native",
        "-Wfatal-errors",
        "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)",
        "-DHWY_IS_TEST",
        "-g",
        "-DSR_DEBUG"
    ],    
    deps = [
        "@hwy",
        "@hwy//:math",
        "@hwy//:hwy_test_util",
        "@hwy//:nanobenchmark",
        "@//src:rand-hw-lib",
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
