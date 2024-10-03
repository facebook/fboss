load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_PHY_IMPLS", "to_sdk_suffix", "to_versions")

COMMON_SRCS = [
    "GalaxyManager.cpp",
    "QsfpRestClient.cpp",
    "Wedge100Manager.cpp",
    "Wedge40Manager.cpp",
    "WedgeManager.cpp",
    "WedgeManagerInit.cpp",
    "BspWedgeManager.cpp",
]

OSS_SRCS = COMMON_SRCS + [
    "oss/WedgeManagerInit.cpp",
    "oss/Wedge400CManager.cpp",
]

CLOSED_SRCS = COMMON_SRCS + [
    "facebook/DarwinManager.cpp",
    "facebook/FujiManager.cpp",
    "facebook/Minipack16QManager.cpp",
    "facebook/Wedge400CManager.cpp",
    "facebook/Wedge400Manager.cpp",
    "facebook/WedgeManagerInit.cpp",
    "facebook/YampManager.cpp",
]

CREDO_SRCS = CLOSED_SRCS + [
    "facebook/CloudRipperManager.cpp",
    "facebook/ElbertMacsecHandler.cpp",
    "facebook/ElbertManager.cpp",
]

COMMON_DEPS = [
    "//fboss/agent/platforms/common/galaxy:galaxy_platform_mapping",
    "//fboss/agent/platforms/common/wedge100:wedge100_platform_mapping",
    "//fboss/agent/platforms/common/wedge40:wedge40_platform_mapping",
    "//fboss/lib/bsp:bsp_core",
    "//fboss/lib/platforms:product-info",
    "//fboss/mka_service/handlers:macsec_handler",
    "//fboss/qsfp_service:qsfp-config",
    "//folly/gen:base",
    "//fboss/lib/bsp/meru400bfu:meru400bfu_bsp",
    "//fboss/lib/bsp/meru400bia:meru400bia_bsp",
    "//fboss/lib/bsp/meru400biu:meru400biu_bsp",
    "//fboss/lib/bsp/montblanc:montblanc_bsp",
    "//fboss/lib/bsp/morgan800cc:morgan800cc_bsp",
]

CLOSED_DEPS = COMMON_DEPS + [
    "//fboss/agent/platforms/common/darwin:darwin_platform_mapping",
    "//fboss/agent/platforms/common/elbert:elbert_platform_mapping",
    "//fboss/agent/platforms/common/fuji:fuji_platform_mapping",
    "//fboss/agent/platforms/common/minipack:minipack_platform_mapping",
    "//fboss/agent/platforms/common/wedge400:wedge400_platform_mapping",
    "//fboss/agent/platforms/common/wedge400:wedge400_platform_utils",
    "//fboss/agent/platforms/common/wedge400c:wedge400c_platform_mapping",
    "//fboss/agent/platforms/common/yamp:yamp_platform_mapping",
    "//fboss/lib/fpga/facebook/fuji:fuji_container",
    "//fboss/lib/phy:phy-management",
    "//fboss/qsfp_service/fsdb:fsdb-syncer",
]

def _get_srcs(sai_impl):
    if sai_impl.name == "credo":
        return CREDO_SRCS
    else:
        # TODO: split up srcs based on sai impl
        return CREDO_SRCS

def _sai_platform_lib(sai_impl):
    impl_suffix = to_sdk_suffix(sai_impl)

    return cpp_library(
        name = "wedge-platform{}".format(impl_suffix),
        srcs = _get_srcs(sai_impl),
        headers = [
            "FbossMacsecHandler.h",
            "QsfpRestClient.h",
            "WedgeManager.h",
            "Wedge400CManager.h",
            "facebook/credo_stub.h",
        ],
        versions = to_versions(sai_impl),
        exported_deps = CLOSED_DEPS + [
            ":wedge-transceiver",
            "//fboss/agent:core",
            "//fboss/lib/phy:sai-phy-management{}".format(impl_suffix),
            "//fboss/qsfp_service:transceiver-manager",
            "//fboss/qsfp_service/module:qsfp-module",
            "//fboss/lib/fpga/facebook/cloudripper:cloudripper_i2c",
            "//fboss/lib/fpga/facebook/darwin:darwin_i2c",
            "//fboss/lib/fpga/facebook/elbert:elbert_i2c",
            "//fboss/lib/fpga:wedge400_i2c",
            "//fboss/lib/fpga/facebook/yamp:yamp_i2c",
            "//fboss/lib/i2c/facebook/fuji:fuji_i2c",
            "//fboss/lib:rest-client",
            "//folly:network_address",
        ],
        exported_external_deps = [
            "boost",
            ("boost", None, "boost_container"),
        ],
    )

def all_platform_libs():
    for sai_impl in SAI_PHY_IMPLS:
        _sai_platform_lib(sai_impl)
