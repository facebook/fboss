"""Hand-written wedge platform library for OSS Bazel build.

The BUCK file uses all_platform_libs() from an internal .bzl macro that
isn't available in the OSS repo. This provides the wedge-platform-default
target that the macro would have generated, matching cmake's
QsfpServicePlatformsWedge.cmake.
"""

load("@rules_cc//cc:defs.bzl", "cc_library")

def wedge_platform_impl():
    """Define wedge-platform-default with all manager sources."""

    cc_library(
        name = "wedge-platform-default",
        srcs = [
            "GalaxyManager.cpp",
            "QsfpRestClient.cpp",
            "Wedge100Manager.cpp",
            "Wedge40Manager.cpp",
            "Wedge400Manager.cpp",
            "Wedge400CManager.cpp",
            "WedgeManager.cpp",
            "WedgeManagerInit.cpp",
            "BspWedgeManager.cpp",
            "oss/WedgeManagerInit.cpp",
            "non_xphy/WedgeManagerInit.cpp",
            "WedgeI2CBusLock.cpp",
            "WedgeQsfp.cpp",
        ],
        hdrs = native.glob(["*.h"]),
        deps = [
            "//fboss/agent/platforms/common:platform_mapping_utils",
            "//fboss/lib:io_stats_recorder",
            "//fboss/lib:rest-client",
            "//fboss/lib:thrift_service_utils",
            "//fboss/lib/bsp:bsp_core",
            "//fboss/lib/fpga:wedge400_i2c",
            "//fboss/lib/usb:base-i2c-dependencies",
            "//fboss/lib/usb:i2-api",
            "//fboss/lib/usb:usb-api",
            "//fboss/lib/usb:wedge_i2c",
            "//fboss/qsfp_service:stats",
            "//fboss/qsfp_service:transceiver-manager",
            "//fboss/qsfp_service/fsdb:fsdb-syncer",
            "//fboss/qsfp_service/module:firmware_upgrader",
            "//fboss/qsfp_service/module:i2c_log_buffer",
            "//fboss/qsfp_service/module:qsfp-module",
            "//fboss/qsfp_service/module/cmis:cmis",
            "@boost//:boost",
            "@fb303//:fb303",
            "@folly//:folly",
            "@glog//:glog",
        ],
        visibility = ["//visibility:public"],
    )
