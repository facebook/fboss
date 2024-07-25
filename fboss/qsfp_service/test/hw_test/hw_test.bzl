load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_PHY_IMPLS", "get_all_native_phy_impls", "to_sdk_suffix", "to_versions")

def _hw_qsfp_ensemble_lib(sai_impl):
    impl_suffix = to_sdk_suffix(sai_impl)

    # for now, use default SAI impl for core if using native SDK
    core_suffix = impl_suffix if (sai_impl.sai_version != None) else "-default"
    return cpp_library(
        name = "hw_qsfp_ensemble{}".format(impl_suffix),
        srcs = [
            "HwQsfpEnsemble.cpp",
        ],
        headers = [
            "HwQsfpEnsemble.h",
        ],
        versions = to_versions(sai_impl),
        exported_deps = [
            "//fboss/agent/platforms/common:platform_mapping",
            "//fboss/agent/test:resourcelibutil",
            "//fboss/lib/fpga:multi_pim_container",
            "//fboss/lib/phy:phy_management_base",
            "//fboss/qsfp_service:core{}".format(core_suffix),
            "//fboss/qsfp_service/platforms/wedge:wedge-platform{}".format(core_suffix),
        ],
    )

def _qsfp_hw_test_lib(sai_impl):
    impl_suffix = to_sdk_suffix(sai_impl)
    return cpp_library(
        name = "hw_qsfp_test_lib-{}-{}".format(sai_impl.name, sai_impl.version),
        srcs = [
            "EmptyHwTest.cpp",
            "HwFirmwareTest.cpp",
            "HwMacsecTest.cpp",
            "HwPimTest.cpp",
            "HwPortProfileTest.cpp",
            "HwPortUtils.cpp",
            "HwTransceiverConfigTest.cpp",
            "HwStatsCollectionTest.cpp",
            "HwTest.cpp",
            "HwTransceiverResetTest.cpp",
            "facebook/HwTransceiverThermalDataTest.cpp",
            "HwExternalPhyPortTest.cpp",
            "HwPortPrbsTest.cpp",
            "HwI2CStressTest.cpp",
            "HwI2cSelectTest.cpp",
            "HwStateMachineTest.cpp",
            "HwTransceiverTest.cpp",
            "OpticsFwUpgradeTest.cpp",
            "HwTransceiverConfigValidationTest.cpp",
        ],
        headers = [
            "HwPortUtils.h",
        ],
        link_whole = True,
        undefined_symbols = True,
        versions = to_versions(sai_impl),
        exported_deps = [
            "fbsource//third-party/googletest:gtest",
            ":hw_qsfp_ensemble{}".format(impl_suffix),
            ":hw_transceiver_utils",
            "//fboss/agent/platforms/common:platform_mapping",
            "//fboss/lib:common_utils",
            "//fboss/lib/fpga:multi_pim_container",
            "//fboss/agent:platform_config-cpp2-types",
            "//fboss/agent:switch_state-cpp2-types",
            "//fboss/lib/phy:phy-cpp2-types",
            "//fboss/mka_service/if:mka_structs-cpp2-types",
            "//fboss/qsfp_service/lib:qsfp-config-parser-helper",
            "//fboss/qsfp_service/module:qsfp-module",
            "//folly/portability:filesystem",
            "//thrift/lib/cpp/util:enum_utils",
        ],
    )

def all_hw_qsfp_ensemble_libs():
    sai_impls = SAI_PHY_IMPLS
    for sai_impl in sai_impls:
        _hw_qsfp_ensemble_lib(sai_impl)
    native_impls = get_all_native_phy_impls()
    for impl in native_impls:
        _hw_qsfp_ensemble_lib(impl)

def all_qsfp_test_libs():
    sai_impls = SAI_PHY_IMPLS
    for sai_impl in sai_impls:
        _qsfp_hw_test_lib(sai_impl)
    native_impls = get_all_native_phy_impls()
    for impl in native_impls:
        _qsfp_hw_test_lib(impl)

def _qsfp_hw_test(impl, versions):
    impl_suffix = to_sdk_suffix(impl)
    return cpp_binary(
        name = "qsfp_hw_test-{}-{}".format(impl.name, impl.version),
        srcs = [
        ],
        auto_headers = AutoHeaders.SOURCES,
        versions = versions,
        deps = [
            "//fboss/agent/hw/test:hw_test_main",
            "//fboss/qsfp_service/test/hw_test:hw_qsfp_ensemble{}".format(impl_suffix),
            "//fboss/qsfp_service/test/hw_test:hw_qsfp_test_lib-{}-{}".format(impl.name, impl.version),
        ],
    )

def all_qsfp_test_binaries():
    sai_impls = SAI_PHY_IMPLS
    for sai_impl in sai_impls:
        _qsfp_hw_test(sai_impl, to_versions(sai_impl))
    native_impls = get_all_native_phy_impls()
    for impl in native_impls:
        _qsfp_hw_test(impl, to_versions(impl))
