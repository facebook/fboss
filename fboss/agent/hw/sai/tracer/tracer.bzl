load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/bcm:wrapped_symbols.bzl", "wrapped_sai_symbols")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_IMPLS", "to_impl_external_deps", "to_impl_suffix", "to_versions")

def _sai_api_tracer_impl(sai_impl):
    sai_external_deps = to_impl_external_deps(sai_impl)
    major, minor, release = sai_impl.sai_version.split(".")
    pp_flags = [] if sai_impl.name == "fake" else [
        "-DSAI_API_VERSION=(100000 * ({}) + 1000 * ({}) + ({}))".format(major, minor, release),
    ]

    cpp_library(
        name = "sai_tracer{}".format(to_impl_suffix(sai_impl)),
        srcs = [
            "AclApiTracer.cpp",
            "BridgeApiTracer.cpp",
            "BufferApiTracer.cpp",
            "CounterApiTracer.cpp",
            "DebugCounterApiTracer.cpp",
            "FdbApiTracer.cpp",
            "HashApiTracer.cpp",
            "HostifApiTracer.cpp",
            "LagApiTracer.cpp",
            "MacsecApiTracer.cpp",
            "MirrorApiTracer.cpp",
            "MplsApiTracer.cpp",
            "NeighborApiTracer.cpp",
            "NextHopApiTracer.cpp",
            "NextHopGroupApiTracer.cpp",
            "PortApiTracer.cpp",
            "QosMapApiTracer.cpp",
            "QueueApiTracer.cpp",
            "RouteApiTracer.cpp",
            "RouterInterfaceApiTracer.cpp",
            "SaiTracer.cpp",
            "SamplePacketApiTracer.cpp",
            "SchedulerApiTracer.cpp",
            "SwitchApiTracer.cpp",
            "SystemPortApiTracer.cpp",
            "TamApiTracer.cpp",
            "TunnelApiTracer.cpp",
            "UdfApiTracer.cpp",
            "Utils.cpp",
            "VirtualRouterApiTracer.cpp",
            "VlanApiTracer.cpp",
            "WredApiTracer.cpp",
        ],
        propagated_pp_flags = pp_flags,
        linker_flags = (
            wrapped_sai_symbols
        ),
        undefined_symbols = True,
        exported_deps = [
            "//fboss/agent:async_logger",
            "//fboss/agent:fboss-error",
            "//fboss/agent:fboss-types",
            "//fboss/agent/hw/sai/api:sai_version",
            "//fboss/agent/hw/sai/api:recursive_glob_headers",
            "//fboss/agent/hw/sai/tracer/run:recursive_glob_headers",
            "//fboss/lib:function_call_time_reporter",
            "//fboss/lib:hw_write_behavior",
            "//fboss/lib:tuple_utils",
            "//folly:singleton",
        ],
        exported_external_deps = sai_external_deps + [
            ("boost", None, "boost_serialization"),
            "glog",
            "gflags",
        ],
        versions = to_versions(sai_impl),
    )

def sai_tracer_apis():
    for sai_impls in SAI_IMPLS:
        _sai_api_tracer_impl(sai_impls)
