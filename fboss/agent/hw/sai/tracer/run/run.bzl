load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_IMPLS", "to_versions")

def _sai_replayer_binary(sai_impl):
    sai_impl_external_deps = []
    if sai_impl.name == "brcm":
        sai_impl_external_deps = [("brcm-sai", None, "sai")]
    elif sai_impl.name == "leaba":
        sai_impl_external_deps = [("leaba-sdk", None, "sai-sdk")]

    return cpp_binary(
        name = "sai_replayer-{}-{}".format(sai_impl.name, sai_impl.version),
        srcs = [
            "Main.cpp",
            "SaiLog.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
        external_deps = [
            ("sai", None),
            ("glibc", None, "rt"),
        ] + sai_impl_external_deps,
    )

def all_replayer_binaries():
    for sai_impl in SAI_IMPLS:
        _sai_replayer_binary(sai_impl)
