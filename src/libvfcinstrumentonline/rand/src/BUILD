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
        "eft.hpp",
        "export_symbols.hpp",
        "main.hpp",
        "rand.hpp",
        "shishua.h",
        "sr.hpp",
        "sr_hw.hpp",
        "ud.hpp",
        "utils.hpp",
        "vector_types.hpp",
        "xoroshiro256+.hpp",
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

cc_binary(
    name = "rand_hwy",
    srcs = [
        "test_sr_hw.cpp",
        "//src:debug.hpp",
        "//src:random-inl.h",
        "//src:sr_hw.hpp",
        "//src:utils.hpp",
        "//src:vector_types.hpp",
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
