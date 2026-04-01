"""OSS config command registration for fboss2-dev and integration tests.

The internal BUCK uses facebook/config/CmdListImpl.cpp which isn't available
in OSS. This provides the OSS equivalent as a cc_library that can be linked
by any binary needing config command registration.
"""

load("@rules_cc//cc:defs.bzl", "cc_library")

def fboss2_config_impl():
    cc_library(
        name = "cmd-list-config-impl-oss",
        srcs = ["oss/config/CmdListImpl.cpp"],
        deps = [
            ":fboss2-config-lib",
            ":fboss2-lib",
        ],
        alwayslink = True,
        visibility = ["//visibility:public"],
    )
