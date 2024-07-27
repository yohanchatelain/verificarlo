COPTS = [
    "-std=c++17",
    "-I.",
]

cc_test(
    name = "test_get_exponent",
    srcs = ["test_get_exponent.cpp"],
    copts = COPTS,
    deps = [
        "//src:rand_headers",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
