load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("//fboss:THIRD-PARTY-VERSIONS.bzl", "BCM_SDKS", "to_impl_suffix", "to_versions")

def bcm_agent_benchmark(name, srcs, extra_deps):
    return [
        cpp_binary(
            name = "{}{}".format(name, to_impl_suffix(sdk)),
            srcs = srcs,
            versions = to_versions(sdk),
            deps = [
                "//fboss/agent/benchmarks:bcm_agent_benchmarks_main{}".format(to_impl_suffix(sdk)),
                "//fboss/agent/hw/bcm/tests:bcm_linkstate_toggler",
                "//fboss/agent/hw/bcm/tests:agent_hw_test_thrift_handler",
                "//fboss/agent/hw/bcm/tests:bcm_ecmp_utils",
            ] + extra_deps,
            external_deps = [
                ("broadcom-xgs-robo", None, "xgs_robo"),
            ],
        )
        for sdk in BCM_SDKS
    ]
