load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_IMPLS", "to_impl_lib_name")

def sai_main_all():
    for sai_impls in SAI_IMPLS:
        cpp_library(
            name = "main-sai-{}".format(to_impl_lib_name(sai_impls)),
            srcs = [
                "Main.cpp",
                "facebook/Main.cpp",
            ],
            exported_deps = [
                ":agent_initializer_h",
                ":fboss_common_init",
                ":fboss_init",
                "//common/init:init",
                "//fboss/agent/facebook:sai_version_{}".format(to_impl_lib_name(sai_impls)),
                "//folly/logging:init",
            ],
        )
