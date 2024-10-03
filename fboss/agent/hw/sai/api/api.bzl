load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_IMPLS", "to_impl_lib_name", "to_impl_suffix", "to_versions")

# The SAI API library depends on a specific SAI Adapter's implementation
# of SAI for the definitions of symbols declared in the header only "sai"
# library. We can try to make the whole implementation of sai api and sai
# switch a huge header only library, but it seems preferable to solve this
# in TARGETS and parameterize building the library by the library that provides
# the crucial sai symbols.

common_srcs = [
    "AddressUtil.cpp",
    "FdbApi.cpp",
    "HashApi.cpp",
    "LoggingUtil.cpp",
    "MplsApi.cpp",
    "NeighborApi.cpp",
    "NextHopGroupApi.cpp",
    "PortApi.cpp",
    "QosMapApi.cpp",
    "RouteApi.cpp",
    "SaiApiLock.cpp",
    "SaiApiTable.cpp",
    "SwitchApi.cpp",
    "SystemPortApi.cpp",
    "Types.cpp",
]

fake_srcs = common_srcs + [
    "fake/FakeSaiExtensions.cpp",
]

brcm_srcs = common_srcs + [
    "bcm/DebugCounterApi.cpp",
    "bcm/PortApi.cpp",
    "bcm/SwitchApi.cpp",
    "bcm/TamApi.cpp",
    "bcm/BufferApi.cpp",
    "bcm/QueueApi.cpp",
]

tajo_srcs = common_srcs + [
    "tajo/DebugCounterApi.cpp",
    "tajo/PortApi.cpp",
    "tajo/TamApi.cpp",
    "tajo/SwitchApi.cpp",
    "tajo/BufferApi.cpp",
    "tajo/QueueApi.cpp",
]

credo_srcs = common_srcs + [
    "oss/DebugCounterApi.cpp",
    "oss/PortApi.cpp",
    "oss/SwitchApi.cpp",
    "oss/TamApi.cpp",
    "oss/BufferApi.cpp",
    "oss/QueueApi.cpp",
]

common_headers = [
    "AclApi.h",
    "AdapterKeySerializers.h",
    "ArsApi.h",
    "ArsProfileApi.h",
    "BridgeApi.h",
    "BufferApi.h",
    "CounterApi.h",
    "DebugCounterApi.h",
    "FdbApi.h",
    "HashApi.h",
    "HostifApi.h",
    "LagApi.h",
    "MacsecApi.h",
    "MirrorApi.h",
    "MplsApi.h",
    "NextHopApi.h",
    "NextHopGroupApi.h",
    "PortApi.h",
    "QueueApi.h",
    "RouteApi.h",
    "RouterInterfaceApi.h",
    "SaiApi.h",
    "SaiApiError.h",
    "SaiAttribute.h",
    "SaiAttributeDataTypes.h",
    "SaiDefaultAttributeValues.h",
    "SaiObjectApi.h",
    "SaiVersion.h",
    "SamplePacketApi.h",
    "SchedulerApi.h",
    "SwitchApi.h",
    "TamApi.h",
    "Traits.h",
    "TunnelApi.h",
    "Types.h",
    "UdfApi.h",
    "VirtualRouterApi.h",
    "VlanApi.h",
    "WredApi.h",
]

common_deps = [
    "fbsource//third-party/fmt:fmt",
    "//fboss/agent:constants",
    "//fboss/agent:fboss-error",
    "//fboss/agent:fboss-types",
    "//fboss/lib:hw_write_behavior",
    "//fboss/lib:function_call_time_reporter",
    "//fboss/lib:tuple_utils",
    "//folly:format",
    "//folly:network_address",
    "//folly:singleton",
    "//folly/logging:logging",
]

common_external_deps = [
    ("boost", None),
    ("boost", None, "boost_variant"),
]

def sai_api_libs():
    all_api_libs = []
    api_srcs = {
        "brcm": brcm_srcs,
        "credo": credo_srcs,
        "fake": fake_srcs,
        "leaba": tajo_srcs,
    }
    for sai_impl in SAI_IMPLS:
        deps = common_deps + [
            "//fboss/agent/hw/sai/impl:{}".format(to_impl_lib_name(sai_impl)),
        ]
        major, minor, release = sai_impl.sai_version.split(".")
        pp_flags = [
            "-DSAI_API_VERSION=(100000 * ({}) + 1000 * ({}) + ({}))".format(major, minor, release),
        ]

        impl_suffix = to_impl_suffix(sai_impl)
        all_api_libs.extend([cpp_library(
            name = "sai_api{}".format(impl_suffix),
            propagated_pp_flags = pp_flags,
            srcs = api_srcs.get(sai_impl.name),
            auto_headers = AutoHeaders.SOURCES,
            headers = common_headers,
            exported_deps = deps,
            exported_external_deps = common_external_deps,
            versions = to_versions(sai_impl),
        )])

    return all_api_libs
