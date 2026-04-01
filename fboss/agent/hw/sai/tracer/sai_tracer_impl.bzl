"""Hand-written SAI tracer library for OSS Bazel build.

Derived from cmake/AgentHwSaiApi.cmake (sai_tracer target).
"""

load("@rules_cc//cc:defs.bzl", "cc_library")

def sai_tracer_impl():
    """Define SAI tracer library."""

    cc_library(
        name = "sai_tracer",
        srcs = native.glob(["*.cpp"]),
        hdrs = native.glob(["*.h"]),
        copts = [
        ],
        deps = [
            "//fboss/agent/hw/sai/api:sai_api",
            "//fboss/agent:async_logger",
            "@folly//:folly",
            "@glog//:glog",
        ],
        visibility = ["//visibility:public"],
    )
