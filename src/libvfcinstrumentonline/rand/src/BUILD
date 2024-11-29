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
    "xoroshiro256+_hw-inl.hpp",
    "xoroshiro256+_hw.h",
    "xoroshiro256+_hw.cpp",
    "sr_hw.cpp",
    "target-utils.h",
])

# Compiler options
COPTS = [
    "-std=c++17",
    "-Wfatal-errors",
]

filegroup(
    name = "srcs-xoroshiro",
    srcs = glob(
        [
            "xoroshiro256+*",
        ] + [
            "random-inl.h",
            "debug_hwy-inl.h",
            "target-utils.h",
        ],
    ),
    visibility = ["//visibility:public"],
)

filegroup(
    name = "srcs-prism",
    srcs = glob(
        [
            "*.cpp",
            "*.hpp",
            "*.h",
        ],
    ),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "prism",
    srcs = [":srcs-prism"],
    hdrs = [
        "//src:sr_hw.h",
        "//src:ud_hw.h",
    ],
    copts = COPTS + [
        "-O2",
        "-Wall",
        "-Wno-psabi",
        "-DHWY_COMPILE_ALL_ATTAINABLE",
    ],
    visibility = ["//visibility:public"],
    deps = ["@hwy"],
)

cc_library(
    name = "prism-dbg",
    srcs = [":srcs-prism"],
    hdrs = [
        "//src:sr_hw.h",
        "//src:ud_hw.h",
    ],
    copts = COPTS + [
        "-O2",
        "-Wall",
        "-Wno-psabi",
        "-DHWY_COMPILE_ALL_ATTAINABLE",
        "-DSR_DEBUG",
        "-DWARN_FMA_EMULATION",
    ],
    visibility = ["//visibility:public"],
    deps = ["@hwy"],
)

cc_library(
    name = "prism-native",
    srcs = [":srcs-prism"],
    hdrs = [
        "//src:sr_hw.h",
        "//src:ud_hw.h",
    ],
    copts = COPTS + [
        "-O2",
        "-Wall",
        "-Wno-psabi",
        "-march=native",
    ],
    visibility = ["//visibility:public"],
    deps = ["@hwy"],
)
