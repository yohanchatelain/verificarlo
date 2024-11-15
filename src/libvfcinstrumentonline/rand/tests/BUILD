load("//tests:macros.bzl", "cc_test_gen", "cc_test_lib_gen")

HWY_OPTS = [
    "-O2",
    "-std=c++20",
    "-Wfatal-errors",
    # "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)",
    # "-DHWY_IS_TEST",
]

DEPS = [
    "@hwy//:math",
]

cc_test_gen(
    name = "get_exponent_test",
    copts = HWY_OPTS,
)

cc_test_gen(
    name = "get_predabs_test",
    copts = HWY_OPTS,
)

cc_test_gen(
    name = "get_pow2_test",
    copts = HWY_OPTS,
)

cc_test_gen(
    name = "get_pow2_test-hw",
    copts = HWY_OPTS + ["-DSR_DEBUG"],
)

cc_test_gen(
    name = "twosum_test",
    copts = HWY_OPTS,
)

cc_test_gen(
    name = "twoprodfma_test",
    copts = HWY_OPTS,
)

cc_test_lib_gen(
    name = "udround_test",
)

cc_test_lib_gen(
    name = "sr_accuracy",
    size = "small",
    src = ["//tests:srround_test.cpp"],
    copts = HWY_OPTS,
)

cc_test_lib_gen(
    name = "sr_accuracy-dbg",
    size = "small",
    src = ["//tests:srround_test.cpp"],
    copts = HWY_OPTS + ["-DSR_DEBUG"],
)

# single test for scalar operations
cc_binary(
    name = "single-test",
    srcs = ["//tests:test_sr_hw.cpp"],
    copts = HWY_OPTS + [
        "-DSR_DEBUG",
    ],
    deps = [
        "@//src:sr",
        "@hwy",
    ],
)

# single test for vector operations
cc_binary(
    name = "single-test-hw",
    srcs = ["//tests:test_sr_hw-API.cpp"],
    copts = HWY_OPTS + [
        "-DSR_DEBUG",
    ],
    deps = [
        "@//src:sr",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@hwy",
        "@hwy//:hwy_test_util",
    ],
)

cc_test_lib_gen(
    name = "xoroshiro",
    size = "medium",
    src = ["//tests:xoroshiro.cpp"],
    copts = [
        "-O2",
        "-Wfatal-errors",
        "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)",
        "-DHWY_IS_TEST",
        "-DSR_DEBUG",
    ],
)

cc_test_lib_gen(
    name = "sr_hw_accuracy",
    size = "medium",
    src = ["//tests:srround_test_hw-inl.cpp"],
    copts = HWY_OPTS,
)

cc_test_lib_gen(
    name = "sr_hw_accuracy-dbg",
    size = "medium",
    src = ["//tests:srround_test_hw-inl.cpp"],
    copts = HWY_OPTS + ["-DSR_DEBUG"],
)

cc_test_lib_gen(
    name = "sr_hw_test",
    size = "medium",
    src = ["//tests:sr_hw_test.cpp"],
    copts = [
        "-std=c++20",
        "-O2",
        "-Wfatal-errors",
        "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)",
        "-DHWY_IS_TEST",
        "-DSR_DEBUG",
        "-Wno-psabi",
    ],
    deps = [
        "@//src:sr",
        "@hwy//:nanobenchmark",
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
