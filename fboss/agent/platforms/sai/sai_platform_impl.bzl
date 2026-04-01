"""Hand-written SAI platform library and wedge_agent binary for OSS Bazel build.

Derived from cmake/AgentPlatformsSai.cmake and platform.bzl (fake_srcs).
Uses oss/ implementations (no proprietary BCM/Tajo sources).
"""

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

# SAI version and BCM defines are set globally in .bazelrc.
# Only add target-specific copts here if needed.
_SAI_COPTS = []

def sai_platform_impl():
    """Define SAI platform library and wedge_agent binary."""

    cc_library(
        name = "sai_platform",
        srcs = native.glob(
            ["*.cpp"],
            exclude = [
                "facebook/*.cpp",
                "bcm/*.cpp",
                "tajo/*.cpp",
                "elbert/*.cpp",
                "SaiPhyPlatform.cpp",
                "BcmRequiredSymbols.cpp",
                "wedge_agent.cpp",
                "WedgeHwAgent.cpp",
            ],
        ) + native.glob(["oss/*.cpp"]),
        hdrs = native.glob(["*.h"]),
        copts = _SAI_COPTS,
        deps = [
            "//fboss/agent:dsfnode_utils",
            "//fboss/agent:handler",
            "//fboss/agent/hw:hw_switch_warmboot_helper",
            "//fboss/agent/hw/sai/switch:sai_switch",
            "//fboss/agent/hw/sai/switch:thrift_handler",
            "//fboss/agent/hw/switch_asics:switch_asics",
            "//fboss/agent/platforms/common:platform_mapping_utils",
            "//fboss/agent/platforms/common/fake_test:fake_test_platform_mapping",
            "//fboss/agent/platforms/common/galaxy:galaxy_platform_mapping",
            "//fboss/agent/platforms/common/minipack:minipack_platform_mapping",
            "//fboss/agent/platforms/common/wedge100:wedge100_platform_mapping",
            "//fboss/agent/platforms/common/wedge40:wedge40_platform_mapping",
            "//fboss/agent/platforms/common/wedge400:wedge400_platform_mapping",
            "//fboss/agent/platforms/common/wedge400:wedge400_platform_utils",
            "//fboss/agent/platforms/common/wedge400c:wedge400c_platform_mapping",
            "//fboss/agent/platforms/common/darwin:darwin_platform_mapping",
            "//fboss/agent/platforms/common/elbert:elbert_platform_mapping",
            "//fboss/agent/platforms/common/fuji:fuji_platform_mapping",
            "//fboss/agent/platforms/common/yamp:yamp_platform_mapping",
            "//fboss/agent/platforms/common/montblanc:montblanc_platform_mapping",
            "//fboss/agent/platforms/common/minipack3bta:minipack3bta_platform_mapping",
            "//fboss/agent/platforms/common/minipack3n:minipack3n_platform_mapping",
            "//fboss/agent/platforms/common/morgan800cc:morgan800cc_platform_mapping",
            "//fboss/agent/platforms/common/janga800bic:janga800bic_platform_mapping",
            "//fboss/agent/platforms/common/tahan800bc:tahan800bc_platform_mapping",
            "//fboss/agent/platforms/common/blackwolf800banw:blackwolf800banw_platform_mapping",
            "//fboss/agent/platforms/common/icecube800banw:icecube800banw_platform_mapping",
            "//fboss/agent/platforms/common/icecube800bc:icecube800bc_platform_mapping",
            "//fboss/agent/platforms/common/icetea800bc:icetea800bc_platform_mapping",
            "//fboss/agent/platforms/common/ladakh800bcls:ladakh800bcls_platform_mapping",
            "//fboss/agent/platforms/common/j4sim:j4sim_platform_mapping",
            "//fboss/agent/platforms/common/wedge800bact:wedge800bact_platform_mapping",
            "//fboss/agent/platforms/common/wedge800cact:wedge800cact_platform_mapping",
            "//fboss/agent/platforms/common/yangra2:yangra2_platform_mapping",
            "//fboss/agent/platforms/common/utils:bcm_yaml_config",
            "//fboss/agent/platforms/common/utils:wedge-led-utils",
            "//fboss/lib/config:fboss_config_utils",
            "//fboss/lib/platforms:product-info",
            "//fboss/qsfp_service/lib:qsfp-service-client",
            "@folly//:folly",
            "@glog//:glog",
        ],
        visibility = ["//visibility:public"],
    )

    cc_binary(
        name = "wedge_agent-sai_impl",
        srcs = ["wedge_agent.cpp"],
        copts = _SAI_COPTS,
        linkopts = [
            "-Wl,--export-dynamic",
            "-Wl,--unresolved-symbols=ignore-all",
            "-Wl,-wrap,sai_api_query",
            "-Wl,-wrap,sai_api_initialize",
            "-Wl,-wrap,sai_api_uninitialize",
            "-Wl,-wrap,sai_get_object_key",
            "-no-pie",
            "-lyaml",
        ],
        deps = [
            ":sai_platform",
            "//fboss/agent:main-common",
            "//fboss/agent:monolithic_agent_initializer",
            "//fboss/agent:setup_thrift_prod",
            "//fboss/agent/hw/sai/api:sai_traced_api",
            "@sai_impl//:sai_impl",
        ],
        visibility = ["//visibility:public"],
    )

    # Split-mode HW agent binary.
    # Derived from cmake/AgentPlatformsSai.cmake _wedge_hwagent_bin function.
    cc_binary(
        name = "fboss_hw_agent-sai_impl",
        srcs = [
            "WedgeHwAgent.cpp",
            "oss/WedgeHwAgent.cpp",
        ],
        copts = _SAI_COPTS,
        linkopts = [
            "-Wl,--export-dynamic",
            "-Wl,--unresolved-symbols=ignore-all",
            "-Wl,-wrap,sai_api_query",
            "-Wl,-wrap,sai_api_initialize",
            "-Wl,-wrap,sai_api_uninitialize",
            "-Wl,-wrap,sai_get_object_key",
            "-no-pie",
            "-lyaml",
        ],
        deps = [
            ":sai_platform",
            "//fboss/agent:fboss_common_init",
            "//fboss/agent:hwagent",
            "//fboss/agent:hwagent-main",
            "//fboss/agent:load_agent_config",
            "//fboss/agent/hw/sai/api:sai_traced_api",
            "//fboss/agent/hw/sai/hw_test:agent_hw_test_thrift_handler",
            "@sai_impl//:sai_impl",
        ],
        visibility = ["//visibility:public"],
    )
