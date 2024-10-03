load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_npu_impls", "get_all_phy_impls", "to_impl_suffix", "to_versions")

_COMMON_SRCS = [
    "ConcurrentIndices.cpp",
    "SaiAclTableGroupManager.cpp",
    "SaiAclTableManager.cpp",
    "SaiArsManager.cpp",
    "SaiArsProfileManager.cpp",
    "SaiBridgeManager.cpp",
    "SaiBufferManager.cpp",
    "SaiCounterManager.cpp",
    "SaiDebugCounterManager.cpp",
    "SaiInSegEntryManager.cpp",
    "SaiFdbManager.cpp",
    "SaiHashManager.cpp",
    "SaiHostifManager.cpp",
    "SaiLagManager.cpp",
    "SaiMacsecManager.cpp",
    "SaiManagerTable.cpp",
    "SaiMirrorManager.cpp",
    "SaiNeighborManager.cpp",
    "SaiNextHopManager.cpp",
    "SaiNextHopGroupManager.cpp",
    "SaiPortManager.cpp",
    "SaiPortUtils.cpp",
    "SaiQosMapManager.cpp",
    "SaiQueueManager.cpp",
    "SaiRouteManager.cpp",
    "SaiRouterInterfaceManager.cpp",
    "SaiRxPacket.cpp",
    "SaiSamplePacketManager.cpp",
    "SaiSchedulerManager.cpp",
    "SaiSwitch.cpp",
    "SaiSwitchManager.cpp",
    "SaiSystemPortManager.cpp",
    "SaiUdfManager.cpp",
    "SaiVlanManager.cpp",
    "SaiVirtualRouterManager.cpp",
    "SaiWredManager.cpp",
    "SaiTunnelManager.cpp",
]

_NPU_COMMON_SRCS = _COMMON_SRCS + [
    "npu/SaiAclTableManager.cpp",
    "npu/SaiPortManager.cpp",
    "npu/SaiSwitch.cpp",
]

_NPU_BRCM_SRCS = _NPU_COMMON_SRCS + [
    "facebook/SaiBufferManager.cpp",
    "facebook/SaiHostifManager.cpp",
    "facebook/SaiSwitchManager.cpp",
    "npu/bcm/SaiQueueManager.cpp",
    "npu/bcm/SaiSwitch.cpp",
    "npu/bcm/SaiSwitchManager.cpp",
    "npu/bcm/SaiPortManager.cpp",
    "npu/bcm/SaiTamManager.cpp",
    "oss/SaiAclTableManager.cpp",
]

_NPU_TAJO_SRCS = _NPU_COMMON_SRCS + [
    "facebook/SaiBufferManager.cpp",
    "facebook/SaiHostifManager.cpp",
    "facebook/SaiSwitchManager.cpp",
    "npu/tajo/SaiPortManager.cpp",
    "npu/tajo/SaiSwitch.cpp",
    "npu/tajo/SaiTamManager.cpp",
    "npu/tajo/SaiAclTableManager.cpp",
    "npu/tajo/SaiSwitchManager.cpp",
    "oss/SaiQueueManager.cpp",
]

_NPU_FAKE_SRCS = _NPU_COMMON_SRCS + [
    "oss/SaiBufferManager.cpp",
    "oss/SaiHostifManager.cpp",
    "oss/SaiSwitch.cpp",
    "oss/SaiTamManager.cpp",
    "oss/SaiPortManager.cpp",
    "oss/SaiAclTableManager.cpp",
    "oss/SaiSwitchManager.cpp",
    "oss/SaiQueueManager.cpp",
]

_PHY_COMMON_SRCS = _COMMON_SRCS + [
    "phy/SaiAclTableManager.cpp",
    "phy/SaiPortManager.cpp",
    "phy/SaiSwitch.cpp",
    "oss/SaiSwitch.cpp",
    "oss/SaiTamManager.cpp",
    "oss/SaiBufferManager.cpp",
    "oss/SaiHostifManager.cpp",
    "oss/SaiPortManager.cpp",
    "oss/SaiAclTableManager.cpp",
    "oss/SaiSwitchManager.cpp",
    "oss/SaiQueueManager.cpp",
]

_PHY_FAKE_SRCS = _PHY_COMMON_SRCS

_PHY_ELBERT_8DD_SRCS = _PHY_COMMON_SRCS

def _sai_switch_srcs(sai_impl, is_npu):
    if is_npu:
        if sai_impl.name == "fake":
            return _NPU_FAKE_SRCS
        if sai_impl.name == "leaba":
            return _NPU_TAJO_SRCS
        if sai_impl.name == "brcm":
            return _NPU_BRCM_SRCS
    else:
        if sai_impl.name == "fake":
            return _PHY_FAKE_SRCS
        if sai_impl.name == "credo":
            return _PHY_ELBERT_8DD_SRCS
    return []

def sai_switch_lib_name(sai_impl, is_npu):
    impl_suffix = to_impl_suffix(sai_impl)
    name_suffix = "switch" if is_npu else "phy"
    return "sai_{}{}".format(name_suffix, impl_suffix)

def sai_switch_dependent_name(name, sai_impl, is_npu):
    impl_suffix = to_impl_suffix(sai_impl)
    switch_type = "" if is_npu else "-phy"
    return "{}{}{}".format(name, switch_type, impl_suffix)

def _sai_switch_lib(sai_impl, is_npu):
    impl_suffix = to_impl_suffix(sai_impl)
    srcs = _sai_switch_srcs(sai_impl, is_npu)
    name = sai_switch_lib_name(sai_impl, is_npu)
    return cpp_library(
        name = name,
        srcs = srcs,
        undefined_symbols = True,
        headers = [
            "SaiTamManager.h",
            "SaiTxPacket.h",
        ],
        exported_deps = [
            "//fboss/agent:core",
            "//fboss/agent/hw:hw_switch_fb303_stats",
            "//fboss/agent/hw:hw_cpu_fb303_stats",
            "//fboss/agent/hw:hw_port_fb303_stats",
            "//fboss/agent/hw:hw_resource_stats_publisher",
            "//fboss/agent/hw:unsupported_feature_manager",
            "//fboss/agent/hw:hw_trunk_counters",
            "//fboss/agent/hw:prbs_stats_entry",
            "//fboss/agent/hw/sai/api:sai_api{}".format(impl_suffix),
            "//fboss/agent/hw/sai/store:sai_store{}".format(impl_suffix),
            "//fboss/agent/platforms/sai:sai_platform_h",
            "//fboss/lib:ref_map",
            "//folly/concurrency:concurrent_hash_map",
            "//folly/container:f14_hash",
            "//thrift/lib/cpp/util:enum_utils",
            "//fboss/lib/phy:phy_utils",
            "//fboss/mka_service/if:mka_structs-cpp2-types",
        ],
        versions = to_versions(sai_impl),
    )

def _handler_lib(sai_impl, is_npu):
    return cpp_library(
        name = "{}".format(sai_switch_dependent_name("thrift_handler", sai_impl, is_npu)),
        srcs = [
            "SaiHandler.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        exported_deps = [
            ":if-cpp2-types",
            ":if-cpp2-services",
            "//fboss/agent/hw/sai/diag:{}".format(sai_switch_dependent_name("diag_shell", sai_impl, is_npu)),
            "//fboss/agent:handler",
        ],
        versions = to_versions(sai_impl),
    )

def all_handler_libs():
    for sai_impl in get_all_npu_impls():
        _handler_lib(sai_impl, True)

    for sai_impl in get_all_phy_impls():
        _handler_lib(sai_impl, False)

def all_npu_libs():
    for sai_impl in get_all_npu_impls():
        _sai_switch_lib(sai_impl, True)

def all_phy_libs():
    for sai_impl in get_all_phy_impls():
        _sai_switch_lib(sai_impl, False)
