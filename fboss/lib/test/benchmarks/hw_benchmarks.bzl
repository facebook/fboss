load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")

def hw_benchmark_lib(name, srcs, extra_deps = None):
    extra_deps = extra_deps if extra_deps else []
    return cpp_library(
        name = name,
        srcs = srcs,
        # Don't drop the benchmark functions they will be needed
        # in the final benchmark run
        link_whole = True,
        # Ensemble will be linked in by particular HwSwitch
        # implementations
        undefined_symbols = True,
        exported_deps = extra_deps + [
            "//fboss/lib/test/benchmarks:hw_benchmark_main",
            "//folly:dynamic",
            "//folly:json",
        ],
    )

def hw_benchmark_binary(name, srcs, extra_deps = None):
    extra_deps = extra_deps if extra_deps else []
    return cpp_binary(
        name = name,
        srcs = srcs,
        deps = extra_deps + [
            "//fboss/lib/test/benchmarks:hw_benchmark_main",
        ],
    )
