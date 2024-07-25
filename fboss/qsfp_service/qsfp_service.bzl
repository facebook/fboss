load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss:THIRD-PARTY-VERSIONS.bzl", "to_versions")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_PHY_IMPLS", "get_all_native_phy_impls", "to_impl_lib_name", "to_sdk_suffix")

def _qsfp_core_lib(impl):
    impl_suffix = to_sdk_suffix(impl)
    deps = [
        ":handler",
        ":stats",
        "//common/services/cpp:acl_checker_module",
        "//common/services/cpp:build_module",
        "//common/services/cpp:fb303_module",
        "//common/services/cpp:thrift_stats_module",
        "//fb303:logging",
        "//folly/experimental:function_scheduler",
        "//folly/io/async:async_signal_handler",
        "//fboss/qsfp_service/platforms/wedge:wedge-platform{}".format(impl_suffix),
    ]

    if impl in SAI_PHY_IMPLS:
        deps.extend(["//fboss/agent/facebook:sai_version_{}".format(to_impl_lib_name(impl))])
    return cpp_library(
        name = "core{}".format(impl_suffix),
        srcs = [
            "facebook/QsfpServer.cpp",
            "QsfpServer.cpp",
            "QsfpServiceSignalHandler.cpp",
        ],
        headers = [
            "QsfpServer.h",
        ],
        undefined_symbols = True,  # TODO(T23121628): fix deps and remove
        versions = to_versions(impl),
        exported_deps = deps,
    )

def _qsfp_service_binary(impl, versions):
    impl_suffix = to_sdk_suffix(impl)

    # for now, use default SAI impl for core if using native SDK
    core_suffix = impl_suffix if impl in SAI_PHY_IMPLS else "-default"
    deps = [
        ":core{}".format(core_suffix),
        "//fboss/qsfp_service/fsdb:fsdb-syncer",
        "//fboss/fsdb/common:flags",
    ]
    return cpp_binary(
        name = "qsfp_service{}".format(impl_suffix),
        srcs = [
            "Main.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        versions = versions,
        preprocessor_flags = [],
        deps = deps,
    )

def all_qsfp_core_libs():
    sai_impls = SAI_PHY_IMPLS
    for sai_impl in sai_impls:
        _qsfp_core_lib(sai_impl)

def all_qsfp_service_binaries():
    sai_impls = SAI_PHY_IMPLS
    for sai_impl in sai_impls:
        _qsfp_service_binary(sai_impl, to_versions(sai_impl))
    native_impls = get_all_native_phy_impls()
    for impl in native_impls:
        _qsfp_service_binary(impl, to_versions(impl))
