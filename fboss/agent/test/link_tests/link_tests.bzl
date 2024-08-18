load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_impls_for", "get_link_group_map", "to_impl_lib_name", "to_impl_suffix", "to_versions")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name")

TEST_BINARY_MODES = ["legacy", "mono", "multi"]

def _sai_link_test_binary(sai_impl, mode):
    impl_suffix = to_impl_suffix(sai_impl)
    test_deps = [
        "//folly/logging:init",
        "//fboss/agent/platforms/sai:{}".format(
            sai_switch_dependent_name("sai_platform", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:sai_port_utils{}".format(impl_suffix),
        "//fboss/agent/hw/sai/hw_test:sai_ecmp_utils{}".format(impl_suffix),
        "//fboss/agent/hw/sai/hw_test:sai_fabric_utils{}".format(impl_suffix),
        "//fboss/agent:main-sai-{}".format(to_impl_lib_name(sai_impl)),
        "//fboss/agent/hw/sai/hw_test:sai_packet_trap_helper{}".format(impl_suffix),
    ]
    if sai_impl.name == "fake" or sai_impl.name == "leaba":
        test_deps.append("//fboss/agent/platforms/sai:bcm-required-symbols")

    if mode == "mono":
        test_deps.append("//fboss/agent/test/link_tests:agent_ensemble_link_tests")
        test_deps.append("//fboss/agent/test:mono_agent_ensemble")
        name = "sai_mono_link_test-{}-{}".format(sai_impl.name, sai_impl.version)
        srcs = [
            "SaiMonoLinkTest.cpp",
        ]
    elif mode == "multi":
        test_deps.append("//fboss/agent/test/link_tests:agent_ensemble_link_tests")
        test_deps.append("//fboss/agent/test:multi_switch_agent_ensemble")
        name = "sai_multi_switch_link_test-{}-{}".format(sai_impl.name, sai_impl.version)
        srcs = ["SaiMultiSwitchLinkTest.cpp"]
    else:
        # default case from before
        test_deps.append("//fboss/agent/test/link_tests:link_tests")
        name = "sai_link_test-{}-{}".format(sai_impl.name, sai_impl.version)
        srcs = [
            "SaiLinkTest.cpp",
        ]

    return cpp_binary(
        name = name,
        srcs = srcs,
        linker_flags = [
            "--export-dynamic",
            "--unresolved-symbols=ignore-all",
        ] if sai_impl.name == "leaba" else [],
        link_group_map = get_link_group_map(name, sai_impl),
        deps = test_deps,
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
    )

def all_sai_link_test_binaries():
    for mode in TEST_BINARY_MODES:
        for sai_impl in get_all_impls_for(True):
            _sai_link_test_binary(sai_impl, mode)
