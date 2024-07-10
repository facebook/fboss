load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_impls_for", "to_impl_suffix", "to_versions")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name")

def _sai_integration_test_binary(sai_impl):
    impl_suffix = to_impl_suffix(sai_impl)
    test_deps = [
        "//fboss/agent/test/integration_tests:agent_integration_test",
        "//fboss/agent/platforms/sai:{}".format(
            sai_switch_dependent_name("sai_platform", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:sai_ecmp_utils{}".format(impl_suffix),
        "//fboss/agent:main-bcm",
        "//folly/io/async:async_signal_handler",
        "//fboss/agent/test:agent_integration_test_base",
    ]
    if sai_impl.name == "fake" or sai_impl.name == "leaba":
        test_deps.append("//fboss/agent/platforms/sai:bcm-required-symbols")
    return cpp_binary(
        name = "sai_agent_integration_test-{}-{}".format(sai_impl.name, sai_impl.version),
        srcs = [
            "SaiAgentIntegrationTests.cpp",
        ],
        deps = test_deps,
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
    )

def _sai_te_integration_test_binary(sai_impl):
    impl_suffix = to_impl_suffix(sai_impl)
    test_deps = [
        "//fboss/agent/test/integration_tests:te_agent_integration_test",
        "//fboss/agent/platforms/sai:{}".format(
            sai_switch_dependent_name("sai_platform", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:sai_ecmp_utils{}".format(impl_suffix),
        "//fboss/agent/hw/sai/hw_test:sai_teflow_utils{}".format(impl_suffix),
        "//fboss/agent:main-bcm",
        "//folly/io/async:async_signal_handler",
        "//fboss/agent/test:agent_integration_test_base",
    ]
    if sai_impl.name == "fake" or sai_impl.name == "leaba":
        test_deps.append("//fboss/agent/platforms/sai:bcm-required-symbols")
    return cpp_binary(
        name = "sai_te_agent_integration_test-{}-{}".format(sai_impl.name, sai_impl.version),
        srcs = [
            "SaiAgentIntegrationTests.cpp",
        ],
        deps = test_deps,
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
    )

def all_sai_integration_test_binaries():
    for sai_impl in get_all_impls_for(True):
        _sai_integration_test_binary(sai_impl)
        _sai_te_integration_test_binary(sai_impl)
