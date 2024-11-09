COPTS = [
    "-std=c++20",
    "-I.",
    "-Wfatal-errors",
]

XOROSHIRO_COPTS = COPTS + []
SHISHUA_COPTS = COPTS + ["-DSHISHUA", "-mavx2", "-O3", "-mfma"]
USE_CXX11_RANDOM_COPTS = COPTS + ["-DUSE_CXX11_RANDOM", "-mavx2", "-O3", "-mfma"]

RNG_COPTS = {
    "XOROSHIRO": XOROSHIRO_COPTS,
    "SHISHUA": SHISHUA_COPTS,
    "CXX11_RANDOM": USE_CXX11_RANDOM_COPTS,
}

DEPS = [
    "//src:rand_headers",
    "@googletest//:gtest",
    "@googletest//:gtest_main",
    "@boost//:math",
    "@boost//:multiprecision",
]

HEADERS = ["//tests:helper.hpp"]

def cc_test_gen(name, src = None, deps = DEPS, copts = COPTS, size = "small"):
    native.cc_test(
        name = name,
        srcs = [src if src else name + ".cpp"] + HEADERS,
        copts = copts,
        deps = deps,
        size = size,
    )

def cc_test_lib_gen(name, rng, src = None, deps = None, copts = COPTS, size = "small"):
    srcs = src if src else [name + ".cpp"]
    srcs += HEADERS
    native.cc_test(
        name = name,
        srcs = srcs,
        copts = copts + RNG_COPTS[rng],
        deps = DEPS + (deps if deps else []),
        size = size,
        visibility = ["//visibility:public"],
    )
