# BUILD file
load("@rules_cc//cc:defs.bzl", "cc_test")

exports_files([
    "Makefile",
    "debug.hpp",
    "debug_hwy-inl.h",
    "eft.hpp",
    "random-inl.h",
    "sr_hw.h",
    "sr.h",
    "sr_hw-inl.h",
    "ud.h",
    "ud_hw.h",
    "ud_hw.cpp",
    "ud_hw-inl.h",
    "utils.hpp",
    "xoroshiro256+_hw.hpp",
    "xoroshiro256+_hw.h",
    "xoroshiro256+_hw.cpp",
    "sr_hw.cpp",
])

# Compiler options
COPTS = [
    "-std=c++17",
    "-Wfatal-errors",
]

filegroup(
    name = "xoroshiro",
    srcs = [
        "debug_hwy-inl.h",
        "random-inl.h",
        "target-utils.h",
        "xoroshiro256+_hw.cpp",
        "xoroshiro256+_hw.h",
        "xoroshiro256+_hw-inl.hpp",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "srcs-sr",
    srcs = glob(
        [
            "*.cpp",
            "*.hpp",
            "*.h",
        ],
        exclude = [
            "ud_hw.cpp",
            "ud_hw.h",
            "ud_hw-inl.h",
            "ud.h",
        ],
    ),
    visibility = ["//visibility:public"],
)

filegroup(
    name = "srcs-ud",
    srcs = glob(
        [
            "*.cpp",
            "*.hpp",
            "*.h",
        ],
        exclude = [
            "sr_hw.cpp",
            "sr_hw.h",
            "sr_hw-inl.h",
            "sr.h",
        ],
    ),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "sr",
    srcs = [":srcs-sr"],
    hdrs = ["//src:sr_hw.h"],
    copts = COPTS + [
        "-O2",
        "-Wall",
        "-Wno-psabi",
    ],
    visibility = ["//visibility:public"],
    deps = ["@hwy"],
)

cc_library(
    name = "sr-native",
    srcs = [":srcs-sr"],
    hdrs = ["//src:sr_hw.h"],
    copts = COPTS + [
        "-O2",
        "-Wall",
        "-Wno-psabi",
        "-march=native",
    ],
    visibility = ["//visibility:public"],
    deps = ["@hwy"],
)

cc_library(
    name = "ud",
    srcs = [":srcs-ud"],
    hdrs = ["//src:ud_hw.h"],
    copts = COPTS + [
        "-O2",
        "-Wall",
        "-Wno-psabi",
    ],
    visibility = ["//visibility:public"],
    deps = ["@hwy"],
)

cc_library(
    name = "ud-native",
    srcs = [":srcs-ud"],
    hdrs = ["//src:ud_hw.h"],
    copts = COPTS + [
        "-O2",
        "-Wall",
        "-Wno-psabi",
        "-march=native",
    ],
    visibility = ["//visibility:public"],
    deps = ["@hwy"],
)

cc_library(
    name = "sr-dbg",
    srcs = [
        "//src:debug.hpp",
        "//src:debug_hwy-inl.h",
        "//src:eft.hpp",
        "//src:random-inl.h",
        "//src:sr.h",
        "//src:sr_hw.cpp",
        "//src:sr_hw-inl.h",
        "//src:utils.hpp",
        "//src:xoroshiro256+_hw.cpp",
        "//src:xoroshiro256+_hw.h",
        "//src:xoroshiro256+_hw-inl.hpp",
    ],
    hdrs = [
        "//src:sr_hw.h",
    ],
    copts = COPTS + [
        "-Og",
        "-g",
        "-Wfatal-errors",
        "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)",
        "-DSR_DEBUG",
        "-Wno-psabi",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@hwy",
        "@hwy//:random",
    ],
)

cc_test(
    name = "test_sr_hw-API",
    timeout = "short",
    srcs = [
        "//src:debug.hpp",
        "//src:debug_hwy-inl.h",
        "//src:random-inl.h",
        "//src:sr_hw-inl.h",
        "//src:test_sr_hw-API.cpp",
        "//src:utils.hpp",
        "//src:xoroshiro256+_hw.hpp",
    ],
    copts = COPTS + [
        "-O2",
        "-march=native",
        "-Wfatal-errors",
        "-DHWY_DISABLED_TARGETS=(HWY_AVX3|HWY_AVX3_ZEN4|HWY_AVX3_SPR)",
        "-DHWY_IS_TEST",
    ],
    deps = [
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@hwy",
        "@hwy//:hwy_test_util",
        "@hwy//:random",
    ],
)
