load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_impls_for", "to_impl_lib_name", "to_impl_suffix", "to_versions")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name")

def _sai_link_test_binary(sai_impl):
    impl_suffix = to_impl_suffix(sai_impl)
    test_deps = [
        "//folly/logging:init",
        "//fboss/agent/test/link_tests:link_tests",
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
    return cpp_binary(
        name = "sai_link_test-{}-{}".format(sai_impl.name, sai_impl.version),
        srcs = [
            "SaiLinkTest.cpp",
        ],
        linker_flags = [
            "--export-dynamic",
            "--unresolved-symbols=ignore-all",
        ] if sai_impl.name == "leaba" else [],
        deps = test_deps,
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
    )

def all_sai_link_test_binaries():
    for sai_impl in get_all_impls_for(True):
        _sai_link_test_binary(sai_impl)
