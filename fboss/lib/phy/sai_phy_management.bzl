load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_PHY_IMPLS", "to_sdk_suffix", "to_versions")

_CREDO_SRCS = [
    "facebook/credo/cloudripper/CloudRipperPhyManager.cpp",
    "facebook/credo/elbert/ElbertPhyManager.cpp",
]

_ALL_SRCS = _CREDO_SRCS

def _get_srcs():
    # TODO: actually split up sources once we support doing so
    return _ALL_SRCS

def _sai_phy_management_lib(sai_impl):
    impl_suffix = to_sdk_suffix(sai_impl)
    return cpp_library(
        name = "sai-phy-management{}".format(impl_suffix),
        srcs = _get_srcs() + [
            "SaiPhyManager.cpp",
        ],
        headers = [
            "SaiPhyManager.h",
        ],
        versions = to_versions(sai_impl),
        exported_deps = [
            "//fboss/lib/phy/facebook:sai_xphy{}".format(impl_suffix),
            "//fboss/lib/phy/facebook/credo:recursive_glob_headers",  # @manual
        ],
    )

def all_sai_phy_management_libs():
    for sai_impl in SAI_PHY_IMPLS:
        _sai_phy_management_lib(sai_impl)
