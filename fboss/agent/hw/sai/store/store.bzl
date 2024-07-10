load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_impls", "to_impl_suffix", "to_versions")

def _sai_store_lib(sai_impl):
    impl_suffix = to_impl_suffix(sai_impl)
    return cpp_library(
        name = "sai_store{}".format(impl_suffix),
        srcs = [
            "SaiObjectEventPublisher.cpp",
            "SaiObject.cpp",
            "SaiStore.cpp",
        ],
        undefined_symbols = True,
        headers = [
            "LoggingUtil.h",
            "SaiObject.h",
            "SaiObjectEventPublisher.h",
            "SaiObjectEventSubscriber.h",
            "SaiObjectEventSubscriber-defs.h",
            "SaiObjectWithCounters.h",
            "SaiStore.h",
            "Traits.h",
        ],
        auto_headers = AutoHeaders.SOURCES,
        exported_deps = [
            "fbsource//third-party/fmt:fmt",
            "//fboss/agent/hw/sai/api:sai_api{}".format(impl_suffix),
            "//fboss/lib:ref_map",
            "//fboss/lib:tuple_utils",
            "//folly:singleton",
        ],
        versions = to_versions(sai_impl),
    )

def sai_store_lib():
    all_impls = get_all_impls()
    for sai_impl in all_impls:
        _sai_store_lib(sai_impl)
