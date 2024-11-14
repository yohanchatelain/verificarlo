COPTS = [
    "-std=c++20",
    "-I.",
    "-Wfatal-errors",
]

SRCS = [
    "//src:srcs",
]

DEPS = [
    "@hwy",
    "@hwy//:hwy_test_util",
    "@googletest//:gtest",
    "@googletest//:gtest_main",
    "@boost//:math",
    "@boost//:multiprecision",
]

HEADERS = ["//tests:helper.hpp"]

def cc_test_gen(name, src = None, deps = DEPS, copts = COPTS, size = "small"):
    native.cc_test(
        name = name,
        srcs = [src if src else name + ".cpp"] + HEADERS + SRCS,
        copts = COPTS + (copts if copts else []),
        deps = deps,
        size = size,
    )

def cc_test_lib_gen(name, src = None, deps = None, copts = COPTS, size = "small"):
    srcs = src if src else [name + ".cpp"]
    srcs += HEADERS
    native.cc_test(
        name = name,
        srcs = srcs + SRCS,
        copts = COPTS + (copts if copts else []),
        deps = DEPS + (deps if deps else []),
        size = size,
        visibility = ["//visibility:public"],
    )
