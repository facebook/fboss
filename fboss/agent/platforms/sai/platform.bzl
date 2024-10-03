load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("@fbcode_macros//build_defs:custom_unittest.bzl", "custom_unittest")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_npu_impls", "get_all_phy_impls", "get_link_group_map", "to_impl_lib_name", "to_impl_suffix", "to_versions")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name", "sai_switch_lib_name")

headers = [
    "SaiBcmDarwinPlatformPort.h",
    "SaiBcmElbertPlatformPort.h",
    "SaiBcmFujiPlatformPort.h",
    "SaiBcmMinipackPlatformPort.h",
    "SaiBcmWedge100PlatformPort.h",
    "SaiBcmYampPlatformPort.h",
    "SaiJanga800bicPlatformPort.h",
    "SaiMeru400bfuPlatformPort.h",
    "SaiMeru400biuPlatformPort.h",
    "SaiMeru800biaPlatformPort.h",
    "SaiMeru800bfaPlatformPort.h",
    "SaiBcmMontblancPlatformPort.h",
    "SaiMeru400biaPlatformPort.h",
    "SaiMorgan800ccPlatformPort.h",
    "SaiTahan800bcPlatformPort.h",
]

common_srcs = [
    "SaiPlatform.cpp",
    "SaiBcmPlatform.cpp",
    "SaiBcmPlatformPort.cpp",
    "SaiBcmWedge100Platform.cpp",
    "SaiBcmWedge400Platform.cpp",
    "SaiBcmWedge400PlatformPort.cpp",
    "SaiBcmDarwinPlatform.cpp",
    "SaiBcmElbertPlatform.cpp",
    "SaiBcmMinipackPlatform.cpp",
    "SaiBcmYampPlatform.cpp",
    "SaiBcmFujiPlatform.cpp",
    "SaiElbert8DDPhyPlatformPort.cpp",
    "SaiFakePlatform.cpp",
    "SaiFakePlatformPort.cpp",
    "SaiJanga800bicPlatform.cpp",
    "SaiJanga800bicPlatformPort.cpp",
    "SaiPlatformPort.cpp",
    "SaiPlatformInit.cpp",
    "SaiWedge400CPlatform.cpp",
    "SaiWedge400CPlatformPort.cpp",
    "SaiTajoPlatform.cpp",
    "SaiTajoPlatformPort.cpp",
    "SaiCloudRipperPlatform.cpp",
    "SaiCloudRipperPlatformPort.cpp",
    "SaiMeru400biuPlatform.cpp",
    "SaiMeru800biaPlatform.cpp",
    "SaiMeru800bfaPlatform.cpp",
    "SaiMeru400bfuPlatform.cpp",
    "SaiMeru400biaPlatformPort.cpp",
    "SaiMeru400biaPlatform.cpp",
    "SaiBcmMontblancPlatform.cpp",
    "SaiBcmMontblancPlatformPort.cpp",
    "SaiMorgan800ccPlatformPort.cpp",
    "SaiMorgan800ccPlatform.cpp",
    "SaiTahan800bcPlatform.cpp",
    "SaiTahan800bcPlatformPort.cpp",
]

bcm_srcs = common_srcs + [
    "bcm/SaiBcmMinipackPlatform.cpp",
    "bcm/SaiBcmPlatform.cpp",
    "bcm/SaiBcmWedge100PlatformPort.cpp",
    "facebook/SaiBcmMinipackPlatformPort.cpp",
    "facebook/SaiBcmFujiPlatformPort.cpp",
    "facebook/SaiBcmWedge400PlatformPort.cpp",
    "facebook/SaiBcmDarwinPlatform.cpp",
    "facebook/SaiBcmDarwinPlatformPort.cpp",
    "facebook/SaiBcmYampPlatformPort.cpp",
    "facebook/SaiBcmElbertPlatformPort.cpp",
    "facebook/SaiMeru400biuPlatform.cpp",
    "facebook/SaiMeru800biaPlatform.cpp",
    "facebook/SaiMeru400bfuPlatform.cpp",
    "facebook/SaiMeru800bfaPlatform.cpp",
    "oss/SaiWedge400CPlatformPort.cpp",
    "facebook/SaiMeru400biuPlatformPort.cpp",
    "facebook/SaiMeru800biaPlatformPort.cpp",
    "facebook/SaiMeru400bfuPlatformPort.cpp",
    "facebook/SaiMeru800bfaPlatformPort.cpp",
    "oss/SaiTajoPlatform.cpp",
    "oss/SaiMeru400biaPlatform.cpp",
    "oss/SaiMorgan800ccPlatformPort.cpp",
]

fake_srcs = common_srcs + [
    "oss/SaiBcmMinipackPlatform.cpp",
    "oss/SaiBcmPlatform.cpp",
    "oss/SaiBcmMinipackPlatformPort.cpp",
    "oss/SaiBcmFujiPlatformPort.cpp",
    "oss/SaiBcmWedge100PlatformPort.cpp",
    "oss/SaiBcmWedge400PlatformPort.cpp",
    "oss/SaiBcmDarwinPlatform.cpp",
    "oss/SaiBcmDarwinPlatformPort.cpp",
    "oss/SaiBcmYampPlatformPort.cpp",
    "oss/SaiBcmElbertPlatformPort.cpp",
    "oss/SaiWedge400CPlatformPort.cpp",
    "oss/SaiMeru400biuPlatform.cpp",
    "oss/SaiMeru800biaPlatform.cpp",
    "oss/SaiMeru400biuPlatformPort.cpp",
    "oss/SaiMeru800biaPlatformPort.cpp",
    "oss/SaiMeru400bfuPlatform.cpp",
    "oss/SaiMeru800bfaPlatform.cpp",
    "oss/SaiMeru400bfuPlatformPort.cpp",
    "oss/SaiMeru800bfaPlatformPort.cpp",
    "oss/SaiTajoPlatform.cpp",
    "oss/SaiMeru400biaPlatform.cpp",
    "oss/SaiMorgan800ccPlatformPort.cpp",
]

tajo_srcs = common_srcs + [
    "oss/SaiBcmMinipackPlatform.cpp",
    "facebook/SaiWedge400CPlatformPort.cpp",
    "oss/SaiBcmPlatform.cpp",
    "oss/SaiBcmMinipackPlatformPort.cpp",
    "oss/SaiBcmFujiPlatformPort.cpp",
    "oss/SaiBcmWedge100PlatformPort.cpp",
    "oss/SaiBcmWedge400PlatformPort.cpp",
    "oss/SaiBcmDarwinPlatform.cpp",
    "oss/SaiBcmDarwinPlatformPort.cpp",
    "oss/SaiBcmYampPlatformPort.cpp",
    "oss/SaiBcmElbertPlatformPort.cpp",
    "oss/SaiMeru400biuPlatform.cpp",
    "oss/SaiMeru800biaPlatform.cpp",
    "oss/SaiMeru800bfaPlatform.cpp",
    "oss/SaiMeru400biuPlatformPort.cpp",
    "oss/SaiMeru800biaPlatformPort.cpp",
    "oss/SaiMeru800bfaPlatformPort.cpp",
    "oss/SaiMeru400bfuPlatform.cpp",
    "oss/SaiMeru400bfuPlatformPort.cpp",
    "tajo/SaiTajoPlatform.cpp",
    "oss/SaiMeru400biaPlatform.cpp",
    "oss/SaiMorgan800ccPlatformPort.cpp",
]

credo_srcs = [
    "elbert/SaiElbert8DDPhyPlatform.cpp",
    "cloudripper/SaiCloudRipperPhyPlatform.cpp",
]

# TODO(ccpowers): Eventually we should be able to split up phy platform
# sources based on the sai impl rather than including all phy sources
phy_srcs = bcm_srcs + credo_srcs

def _get_srcs(sai_impl):
    if not sai_impl:
        return fake_srcs
    if sai_impl.name == "brcm":
        return bcm_srcs
    if sai_impl.name == "leaba":
        return tajo_srcs
    if sai_impl.name == "credo":
        return phy_srcs
    return fake_srcs

# sai_impl should match one of the names defined in sai/api/TARGETS
# Note: that TARGETS file has a comment explaining the design for the SAI
# library TARGETS.
def _sai_platform_lib(sai_impl, is_npu = True):
    netwhoami_deps = [
        "//neteng/netwhoami/lib/cpp:recover",
    ]

    return cpp_library(
        name = "{}".format(sai_switch_dependent_name("sai_platform", sai_impl, is_npu)),
        srcs = _get_srcs(sai_impl),
        headers = headers,
        versions = to_versions(sai_impl),
        exported_deps = netwhoami_deps + [
            "//fboss/agent:handler",
            "//fboss/agent/state:state",
            "//fboss/lib/platforms:platform_mode",
            "//fboss/lib/platforms:product-info",
            "//fboss/agent/hw/sai/switch:{}".format(sai_switch_lib_name(sai_impl, is_npu)),
            "//fboss/agent/hw/sai/switch:{}".format(sai_switch_dependent_name("thrift_handler", sai_impl, is_npu)),
            "//fboss/agent/hw:hw_switch_warmboot_helper",
            "//fboss/agent/hw/switch_asics:switch_asics",
            "//fboss/agent/platforms/common/elbert:elbert_platform_mapping",
            "//fboss/agent/platforms/common/fake_test:fake_test_platform_mapping",
            "//fboss/agent/platforms/common/galaxy:galaxy_platform_mapping",
            "//fboss/agent/platforms/common/wedge100:wedge100_platform_mapping",
            "//fboss/agent/platforms/common/wedge40:wedge40_platform_mapping",
            "//fboss/agent/platforms/common/wedge400c:wedge400c_platform_mapping",
            "//fboss/agent/platforms/common/wedge400:wedge400_platform_mapping",
            "//fboss/agent/platforms/common/wedge400:wedge400_platform_utils",
            "//fboss/agent/platforms/common/darwin:darwin_platform_mapping",
            "//fboss/agent/platforms/common/wedge400:wedge400_platform_utils",
            "//fboss/agent/platforms/common/minipack:minipack_platform_mapping",
            "//fboss/agent/platforms/common/yamp:yamp_platform_mapping",
            "//fboss/agent/platforms/common/fuji:fuji_platform_mapping",
            "//fboss/agent/platforms/common/elbert:elbert_platform_mapping",
            "//fboss/agent/platforms/common/meru400biu:meru400biu_platform_mapping",
            "//fboss/agent/platforms/common/meru400bfu:meru400bfu_platform_mapping",
            "//fboss/agent/platforms/common/meru400bia:meru400bia_platform_mapping",
            "//fboss/agent/platforms/common/montblanc:montblanc_platform_mapping",
            "//fboss/agent/platforms/common/morgan800cc:morgan800cc_platform_mapping",
            "//fboss/agent/hw/test:config_factory",
            "//fboss/qsfp_service/lib:qsfp-service-client",
            "//fboss/lib/config:fboss_config_utils",
            "//fboss/agent/platforms/common/utils:wedge-led-utils",
            "//fboss/agent/platforms/common/utils:bcm_yaml_config",
            "//fboss/lib/fpga:wedge400_fpga",
            "//fboss/lib/fpga/facebook/darwin:darwin_fpga",
            "//fboss/lib/fpga/facebook/cloudripper:cloudripper_fpga",
            "//fboss/lib/fpga/facebook/yamp:yamp_fpga",
            "//fboss/agent/platforms/common/cloud_ripper:cloud_ripper_platform_mapping",
        ],
    )

def _versions(sai_impl):
    versions = to_versions(sai_impl)
    return versions

def _wedge_agent_bin(sai_impl, is_npu):
    wedge_agent_name = "wedge_agent{}".format(to_impl_suffix(sai_impl))
    sai_platform_name = sai_switch_dependent_name("sai_platform", sai_impl, is_npu)
    return cpp_binary(
        name = "{}".format(wedge_agent_name),
        srcs = [
            "wedge_agent.cpp",
        ],
        linker_flags = [
            "--export-dynamic",
            "--unresolved-symbols=ignore-all",
        ],
        link_group_map = get_link_group_map(wedge_agent_name, sai_impl),
        auto_headers = AutoHeaders.SOURCES,
        versions = _versions(sai_impl),
        deps = [
            "//fboss/agent:monolithic_agent_initializer",
            "//fboss/agent:main-sai-{}".format(to_impl_lib_name(sai_impl)),
            ":{}".format(sai_platform_name),
            "//fboss/agent/hw/sai/impl:{}".format(to_impl_lib_name(sai_impl)),
        ],
    )

def get_version_info_test_env(sai_impl):
    test_env = {}
    if sai_impl.is_dyn:
        test_env["LD_LIBRARY_PATH"] = "third-party-buck/platform010-compat/build/{}/{}/lib/dyn".format(sai_impl.sdk_name, sai_impl.version)
    return test_env

def _wedge_agent_version_info_test(sai_impl, wedge_agent_name_prefix):
    wedge_agent_name = "{}{}".format(wedge_agent_name_prefix, to_impl_suffix(sai_impl))
    sai_platform_name = sai_switch_dependent_name("sai_platform", sai_impl, True)
    return custom_unittest(
        name = "test-{}-version".format(wedge_agent_name),
        command = [
            "fboss/lib/test/test_version.sh",
            "$(exe_target :{})".format(wedge_agent_name),
        ],
        type = "simple",
        versions = _versions(sai_impl) if sai_impl.name != "fake" else {},
        deps = [
            "//fboss/agent:main-sai-{}".format(to_impl_lib_name(sai_impl)),
            ":{}".format(sai_platform_name),
            "//fboss/agent/hw/sai/impl:{}".format(to_impl_lib_name(sai_impl)),
        ],
        env = get_version_info_test_env(sai_impl),
    )

def _wedge_hwagent_bin(sai_impl, hwagent_prefix):
    wedge_agent_name = "{}{}".format(hwagent_prefix, to_impl_suffix(sai_impl))
    sai_platform_name = sai_switch_dependent_name("sai_platform", sai_impl, True)
    hw_bin_deps = [
        "//fboss/agent:hwagent-main",
        "//fboss/agent/facebook:sai_version_{}".format(to_impl_lib_name(sai_impl)),
        ":{}".format(sai_platform_name),
        "//fboss/agent/hw/sai/impl:{}".format(to_impl_lib_name(sai_impl)),
        "//fboss/agent/hw/sai/hw_test:{}".format(sai_switch_dependent_name("agent_hw_test_thrift_handler", sai_impl, True)),
    ]
    if sai_impl.name == "fake" or sai_impl.name == "leaba":
        hw_bin_deps.append("//fboss/agent/platforms/sai:bcm-required-symbols")
    return cpp_binary(
        name = "{}".format(wedge_agent_name),
        srcs = [
            "WedgeHwAgent.cpp",
            "facebook/WedgeHwAgent.cpp",
        ],
        linker_flags = [
            "--export-dynamic",
            "--unresolved-symbols=ignore-all",
        ],
        link_group_map = get_link_group_map(wedge_agent_name, sai_impl),
        auto_headers = AutoHeaders.SOURCES,
        versions = _versions(sai_impl),
        deps = hw_bin_deps,
    )

def all_wedge_agent_bins():
    for sai_impl in get_all_npu_impls():
        _sai_platform_lib(sai_impl, True)
        _wedge_agent_bin(sai_impl, True)
        _wedge_agent_version_info_test(sai_impl, "wedge_agent")
        _wedge_agent_version_info_test(sai_impl, "fboss_hw_agent")
        _wedge_hwagent_bin(sai_impl, "fboss_hw_agent")

    for sai_impl in get_all_phy_impls():
        _sai_platform_lib(sai_impl, False)
