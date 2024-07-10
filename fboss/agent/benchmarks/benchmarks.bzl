load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss:THIRD-PARTY-VERSIONS.bzl", "BCM_SDKS", "to_impl_suffix", "to_versions")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_npu_impls", "to_impl_lib_name")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name")

def bcm_agent_benchmark_libs(srcs):
    return [
        cpp_library(
            name = "bcm_agent_benchmarks_main{}".format(to_impl_suffix(sdk)),
            srcs = srcs,
            headers = [
                "AgentBenchmarksMain.h",
            ],
            versions = to_versions(sdk),
            exported_deps = [
                ":mono_agent_benchmarks",
                "//fboss/agent:main-bcm",
                "//fboss/agent/platforms/wedge:platform",
                "//folly:benchmark",
                "//folly:dynamic",
                "//folly/init:init",
                "//folly/logging:init",
            ],
        )
        for sdk in BCM_SDKS
    ]

def sai_agent_mono_benchmark_libs(srcs):
    libs = []
    prefix = "mono_sai_agent_benchmarks_main"
    benchmarks_dep = ":mono_agent_benchmarks"
    for sai_impl in get_all_npu_impls():
        name = sai_switch_dependent_name(prefix, sai_impl, True)
        platform = sai_switch_dependent_name("sai_platform", sai_impl, True)
        libs += [
            cpp_library(
                name = name,
                srcs = srcs,
                headers = [
                    "AgentBenchmarksMain.h",
                ],
                versions = to_versions(sai_impl),
                exported_deps = [
                    benchmarks_dep,
                    "//fboss/agent:main-sai-{}".format(to_impl_lib_name(sai_impl)),
                    "//fboss/agent/platforms/sai:{}".format(platform),
                    "//folly:benchmark",
                    "//folly:dynamic",
                    "//folly/init:init",
                    "//folly/logging:init",
                ],
            ),
        ]

    return libs

def sai_agent_multi_switch_benchmark_lib(srcs):
    name = "multi_switch_sai_agent_benchmarks_main"
    benchmarks_dep = ":multi_switch_agent_benchmarks"
    return cpp_library(
        name = name,
        srcs = srcs,
        headers = [
            "AgentBenchmarksMain.h",
        ],
        exported_deps = [
            benchmarks_dep,
            "//folly:benchmark",
            "//folly:dynamic",
            "//folly/init:init",
            "//folly/logging:init",
        ],
    )

def agent_benchmark_lib(mono):
    name = "mono_agent_benchmarks" if mono else "multi_switch_agent_benchmarks"
    ensemble_lib = "//fboss/agent/test:mono_agent_ensemble" if mono else "//fboss/agent/test:multi_switch_agent_ensemble"
    return cpp_library(
        name = name,
        srcs = [
            "AgentBenchmarks.cpp",
        ],
        headers = [
            "AgentBenchmarks.h",
        ],
        exported_deps = [
            "fbsource//third-party/googletest:gtest",
            ensemble_lib,
        ],
    )

def agent_benchmark_libs():
    return [
        agent_benchmark_lib(True),
        agent_benchmark_lib(False),
    ]
