load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")

def agent_benchmark_lib(name, srcs, extra_deps = None):
    extra_deps = extra_deps if extra_deps else []
    extra_deps += [
        ":hw_route_scale_benchmark_helpers",
        "//folly:benchmark",
        "//folly:dynamic",
        "//folly:json",
        "//fboss/agent/hw/test:config_factory",
        "//fboss/agent/hw/test:hw_packet_utils",
        "//fboss/agent/test/utils:voq_test_utils",
        "//fboss/agent/test:ecmp_helper",
        "//fboss/lib:function_call_time_reporter",
        "//fboss/agent/benchmarks:agent_benchmarks_h",
    ]

    return cpp_library(
        name = name,
        srcs = srcs,
        # Don't drop the benchmark functions they will be needed
        # in the final benchmark run
        link_whole = True,
        # Ensemble will be linked in by particular HwSwitch
        # implementations
        undefined_symbols = True,
        exported_deps = extra_deps,
    )
