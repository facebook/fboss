load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")

def _get_oss_suffix(oss):
    return "-oss" if oss else ""

common_srcs = []

oss_srcs = [
    "oss/BcmLedUtils.cpp",
]

facebook_srcs = [
    "facebook/BcmLedUtils.cpp",
]

def _bcm_led_utils(oss):
    srcs = []
    deps = [
        "//fboss/agent:fboss-error",
        "//folly:range",
    ]
    external_deps = []
    if oss:
        srcs = common_srcs + oss_srcs
        external_deps = [
            "glog",
        ]
    else:
        srcs = common_srcs + facebook_srcs
        external_deps = [
            "glog",
            ("broadcom-xgs-robo", None, "xgs_robo"),
        ]
        deps = deps + [
            "//fboss/agent/platforms/common/utils:wedge-led-utils",
        ]
    oss_suffix = _get_oss_suffix(oss)
    return cpp_library(
        name = "bcm-led-utils{}".format(oss_suffix),
        srcs = srcs,
        auto_headers = AutoHeaders.SOURCES,
        headers = [
            "BcmLedUtils.h",
        ],
        exported_deps = deps,
        exported_external_deps = external_deps,
    )

def bcm_led_utils():
    for oss in [True, False]:
        _bcm_led_utils(oss)
