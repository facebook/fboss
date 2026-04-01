"""Hand-written SAI diag library for OSS Bazel build.

Derived from cmake/AgentHwSaiDiag.cmake.
"""

load("@rules_cc//cc:defs.bzl", "cc_library")

def sai_diag_impl():
    """Define SAI diag library with sources."""

    cc_library(
        name = "diag_lib",
        srcs = native.glob(["*.cpp"]) + native.glob(["oss/*.cpp"]),
        hdrs = native.glob(["*.h"]),
        copts = [
            "-isystem/usr/include/python3.12",
        ],
        linkopts = ["-lpython3.12"],
        deps = [
            "//fboss/agent/hw/sai/api:sai_api",
            "//fboss/agent/hw/sai/switch:if",
            "//fboss/agent/hw/sai/switch:sai_switch",
            "@fb303//:fb303",
            "@folly//:folly",
            "@glog//:glog",
            "@googletest//:googletest",
        ],
        visibility = ["//visibility:public"],
    )
