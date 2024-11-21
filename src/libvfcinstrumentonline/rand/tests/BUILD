load("//tests:macros.bzl", "cc_test_gen", "cc_test_lib_gen")

HWY_OPTS = [
    "-O2",
]

cc_test_gen(
    name = "test_get_exponent",
    copts = HWY_OPTS,
)

cc_test_gen(
    name = "test_get_predabs",
    copts = HWY_OPTS,
)

cc_test_gen(
    name = "test_get_pow2",
    copts = HWY_OPTS,
)

cc_test_gen(
    name = "test_get_pow2_hwy",
    copts = HWY_OPTS,
    dbg = True,
)

cc_test_gen(
    name = "test_twosum",
    copts = HWY_OPTS,
)

cc_test_gen(
    name = "test_twoprodfma",
    copts = HWY_OPTS,
)

# Scalar tests

cc_test_lib_gen(
    name = "sr-scalar-accuracy",
    size = "small",
    src = ["//tests:test_sr_accuracy.cpp"],
    copts = HWY_OPTS,
)

cc_test_lib_gen(
    name = "sr-scalar-accuracy-dbg",
    size = "small",
    src = ["//tests:test_sr_accuracy.cpp"],
    copts = HWY_OPTS,
    dbg = True,
)

cc_test_lib_gen(
    name = "ud-scalar-accuracy",
    size = "small",
    src = [
        "//tests:test_ud_accuracy.cpp",
        "//src:srcs-ud",
    ],
    copts = HWY_OPTS,
)

cc_binary(
    name = "sr-scalar-single-test",
    srcs = ["//tests:test_single_sr.cpp"],
    copts = HWY_OPTS + ["-DSR_DEBUG"],
    deps = ["@//src:sr"],
)

cc_binary(
    name = "ud-scalar-single-test",
    srcs = ["//tests:test_single_ud.cpp"],
    copts = HWY_OPTS + ["-DSR_DEBUG"],
    deps = ["@//src:ud"],
)

# Vector tests

cc_test_lib_gen(
    name = "xoroshiro",
    size = "medium",
    src = [
        "//src:xoroshiro",
        "//tests:test_xoroshiro.cpp",
    ],
    copts = HWY_OPTS,
    dbg = True,
)

cc_binary(
    name = "sr-vector-single-test",
    srcs = ["//tests:test_single_sr_hwy.cpp"],
    copts = HWY_OPTS + ["-DSR_DEBUG"],
)

cc_test_lib_gen(
    name = "sr-vector-accuracy",
    size = "medium",
    src = ["//tests:test_sr_hwy_accuracy.cpp"],
    copts = HWY_OPTS,
)

cc_test_lib_gen(
    name = "ud-vector-accuracy",
    size = "medium",
    src = [
        "//tests:test_ud_hwy_accuracy.cpp",
        "//src:srcs-ud",
    ],
    copts = HWY_OPTS,
)

cc_test_lib_gen(
    name = "sr-vector-accuracy-dbg",
    size = "medium",
    src = ["//tests:test_sr_hwy_accuracy.cpp"],
    copts = HWY_OPTS,
    dbg = True,
)

cc_test_lib_gen(
    name = "sr-perf",
    size = "medium",
    src = ["//tests:test_sr_hwy_performance.cpp"],
    copts = HWY_OPTS,
    deps = [
        "@//src:sr",
        "@hwy//:nanobenchmark",
    ],
)

cc_test_lib_gen(
    name = "ud-perf",
    size = "medium",
    src = ["//tests:test_ud_hwy_performance.cpp"],
    copts = HWY_OPTS,
    deps = [
        "@//src:ud",
        "@hwy//:nanobenchmark",
    ],
)

test_suite(
    name = "all",
    tests = [
        ":get_exponent_test",
        ":get_pow2_test",
        ":get_predabs_test",
        ":twoprodfma_test",
        ":twosum_test",
        ":udround_test",
    ],
)
