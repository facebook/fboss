load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/bcm:wrapped_symbols.bzl", "wrapped_bcm_sdk_symbols", "wrapped_bcm_symbols", "wrapped_soc_symbols_tmp")

def sdk_tracer_lib(link_whole):
    link_whole_suffix = "_link_whole" if link_whole else ""

    return cpp_library(
        name = "sdk_tracer{}".format(link_whole_suffix),
        srcs = [
            "SdkTracer.cpp",
        ],
        compiler_flags = [
            "-DSOC_PCI_DEBUG",
        ],
        link_whole = link_whole,
        linker_flags = (
            wrapped_bcm_sdk_symbols + wrapped_bcm_symbols + wrapped_soc_symbols_tmp
        ),
        exported_deps = [
            ":sdk_wrap_settings",
            "//fboss/agent/hw/bcm:bcm_cinter",
            "//fboss/lib:function_call_time_reporter",
            "//folly/io/async:async_base",
        ],
        exported_external_deps = [
            "glog",
            "gflags",
        ],
    )
