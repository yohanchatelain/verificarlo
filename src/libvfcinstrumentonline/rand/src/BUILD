# BUILD file

# Compiler options
COPTS = [
    "-std=c++20",
    "-mavx2",
    "-O3",
    "-DXOROSHIRO",  # Equivalent to -D$(RNG) where RNG=XOROSHIRO
    "-I.",
]

cc_library(
    name = "rand_headers",
    hdrs = [
        "debug.hpp",
        "debug_hwy-inl.h",
        "eft.hpp",
        "export_symbols.hpp",
        "main.hpp",
        "rand.hpp",
        "random-inl.h",
        "sr_hw.h",
        "shishua.h",
        "sr.hpp",
        "sr_hw-inl.h",
        "ud.hpp",
        "utils.hpp",
        "vector_types.hpp",
        "xoroshiro256+.hpp",
        "xoroshiro256+_hw.hpp",
    ],
    visibility = ["//visibility:public"],
)

# Library target for rand.cpp
cc_library(
    name = "rand",
    srcs = [
        "debug.hpp",
        "eft.hpp",
        "main.cpp",
        "rand.hpp",
        "sr.hpp",
        "ud.hpp",
        "utils.hpp",
        "vector_types.hpp",
        "xoroshiro256+.hpp",
    ],
    copts = COPTS,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "rand_hw",
    srcs = [
        "debug.hpp",
        "eft.hpp",
        "main.cpp",
        "rand.hpp",
        "sr_hw.h",
        "sr_hw-inl.h",
        "debug_hwy-inl.h",
        "xoroshiro256+_hw.hpp",
        "random-inl.h",
        "sr_hw.cpp",
        "shishua.h",
        "sr.hpp",
        "ud.hpp",
        "utils.hpp",
        "vector_types.hpp",
        "xoroshiro256+.hpp",
    ],
    copts = COPTS,
    deps = [
        "@hwy",
        "@hwy//:math",
    ],
)

cc_library(
    name = "sr_hw",
    srcs = [
        "sr_hw.cpp",
        "//src:debug.hpp",
        "//src:debug_hwy-inl.h",
        "//src:random-inl.h",
        "//src:sr_hw-inl.h",
        "//src:sr_hw.h",
        "//src:utils.hpp",
        "//src:xoroshiro256+_hw.hpp",
    ],
    copts = COPTS + [
        "-DXOROSHIRO",
        "-mavx2",
        "-O2",
        "-Wfatal-errors",
    ],
    deps = [
        "@hwy",
        "@hwy//:random",
    ],
    visibility = ["//visibility:public"]
)


cc_binary(
    name = "rand_hwy",
    srcs = [
        "test_sr_hw.cpp",
        "//src:debug.hpp",
        "//src:debug_hwy-inl.h",
        "//src:random-inl.h",
        "//src:sr_hw-inl.h",
        "//src:utils.hpp",
        "//src:xoroshiro256+_hw.hpp",
    ],
    copts = COPTS + [
        "-DXOROSHIRO",
        "-mavx2",
        "-O2",
        "-Wfatal-errors",
    ],
    deps = [
        "@hwy",
        "@hwy//:random",
    ],
)

cc_test(
    name = "test_sr_hw-API",
    srcs = [
        "//src:test_sr_hw-API.cpp",
        "//src:debug.hpp",
        "//src:debug_hwy-inl.h",
        "//src:random-inl.h",
        "//src:sr_hw-inl.h",
        "//src:utils.hpp",
        "//src:xoroshiro256+_hw.hpp",
    ],
    timeout = "short",
    copts = COPTS + [
        "-O2",
        "-Wfatal-errors",
        "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)"
    ],
    local_defines = [
        "HWY_IS_TEST",
    ],
    deps = [
        "@hwy",
        "@hwy//:random",
        "@hwy//:hwy_test_util",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
