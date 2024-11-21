"""
"""

COPTS = [
    "-std=c++17",
    "-I.",
    "-Wfatal-errors",
    "-DHWY_IS_TEST",
]

SRCS = []

DEPS = [
    "@hwy",
    "@hwy//:hwy_test_util",
    "@googletest//:gtest",
    "@googletest//:gtest_main",
    "@boost//:math",
    "@boost//:multiprecision",
]

HEADERS = ["//tests:helper.hpp"]

def cc_test_gen(name, src = None, deps = DEPS, copts = COPTS, size = "small", dbg = False):
    native.cc_test(
        name = name,
        srcs = [src if src else name + ".cpp"] + HEADERS + SRCS,
        copts = COPTS + (copts if copts else []) + (["-DSR_DEBUG"] if dbg else []),
        deps = deps,
        size = size,
    )

def cc_test_lib_gen(name, src = None, deps = None, copts = COPTS, size = "small", dbg = False):
    srcs = src if src else [name + ".cpp"]
    srcs += HEADERS
    native.cc_test(
        name = name,
        srcs = srcs + SRCS,
        copts = COPTS + (copts if copts else []) + (["-DSR_DEBUG"] if dbg else []),
        deps = DEPS + (deps if deps else []),
        size = size,
        visibility = ["//visibility:public"],
    )
