load("@fbcode_macros//build_defs:cpp_unittest.bzl", "cpp_unittest")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_FAKE_IMPLS", "to_impl_lib_name", "to_impl_suffix")

def api_unittest(name, srcs, deps = None, supports_static_listing = True):
    if deps == None:
        deps = []
    for sai_impl in SAI_FAKE_IMPLS:
        api_suffix = to_impl_suffix(sai_impl)
        cpp_unittest(
            name = "{}-{}".format(name, sai_impl.name),
            srcs = srcs,
            supports_static_listing = supports_static_listing,
            deps = [
                "fbsource//third-party/googletest:gtest",
                "//fboss/agent/hw/sai/api:sai_api{}".format(api_suffix),
                "//fboss/agent/hw/sai/impl:{}".format(to_impl_lib_name(sai_impl)),
            ] + deps,
        )
