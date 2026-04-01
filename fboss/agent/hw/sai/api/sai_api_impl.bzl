"""Hand-written SAI API library targets for OSS Bazel build.

Source file lists derived from Meta's internal api.bzl.
Uses bcm/ variant sources for the BCM SAI SDK.
"""

load("@rules_cc//cc:defs.bzl", "cc_library")

# SAI version and BCM defines are set globally in .bazelrc.
_SAI_COPTS = []

# Matches common_srcs from Meta's api.bzl
_COMMON_SRCS = [
    "AddressUtil.cpp",
    "FdbApi.cpp",
    "SaiAttribute.cpp",
    "HashApi.cpp",
    "LoggingUtil.cpp",
    "MplsApi.cpp",
    "NeighborApi.cpp",
    "NextHopGroupApi.cpp",
    "PortApi.cpp",
    "QosMapApi.cpp",
    "RouteApi.cpp",
    "SaiApiLock.cpp",
    "Srv6Api.cpp",
    "SaiApiTable.cpp",
    "SwitchApi.cpp",
    "SystemPortApi.cpp",
    "Types.cpp",
]

# Matches brcm_srcs from Meta's api.bzl
_BRCM_SRCS = _COMMON_SRCS + [
    "bcm/AclApi.cpp",
    "bcm/ArsApi.cpp",
    "bcm/ArsProfileApi.cpp",
    "bcm/DebugCounterApi.cpp",
    "bcm/NextHopGroupApi.cpp",
    "bcm/PortApi.cpp",
    "bcm/SwitchApi.cpp",
    "bcm/TamApi.cpp",
    "bcm/BufferApi.cpp",
    "bcm/QueueApi.cpp",
    "bcm/MirrorApi.cpp",
    "bcm/SystemPortApi.cpp",
    "bcm/HostifApi.cpp",
]

def sai_api_impl():
    """Define SAI API and traced API libraries."""

    cc_library(
        name = "sai_api",
        srcs = _BRCM_SRCS,
        hdrs = native.glob(["*.h"]) + native.glob(["fake/*.h"]),
        copts = _SAI_COPTS,
        deps = [
            ":address_util",
            ":sai_types",
            "//fboss/agent:fboss-error",
            "//fboss/agent:fboss-types",
            "//fboss/lib:function_call_time_reporter",
            "//fboss/lib:hw_write_behavior",
            "@folly//:folly",
            "@libsai//:libsai",
        ],
        visibility = ["//visibility:public"],
    )

    cc_library(
        name = "sai_traced_api",
        copts = _SAI_COPTS,
        deps = [
            ":sai_api",
            "//fboss/agent/hw/sai/tracer:sai_tracer",
        ],
        visibility = ["//visibility:public"],
    )
