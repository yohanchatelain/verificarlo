# BUILD file

# Compiler options
COPTS = [
    "-std=c++17",
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
        "rand.hpp",
        "sr.hpp",
        "ud.hpp",
        "utils.hpp",
        "xoroshiro128+.hpp",
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
        "xoroshiro128+.hpp",
    ],
    copts = COPTS,
    visibility = ["//visibility:public"],
)
