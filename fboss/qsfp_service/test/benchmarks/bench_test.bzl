load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "SAI_PHY_IMPLS", "get_all_native_phy_impls", "to_sdk_suffix", "to_versions")

def _hw_bench_utils_lib(sai_impl):
    impl_suffix = to_sdk_suffix(sai_impl)

    # for now, use default SAI impl for core if using native SDK
    core_suffix = impl_suffix if (sai_impl.sai_version != None) else "-default"
    return cpp_library(
        name = "hw_bench_utils{}".format(impl_suffix),
        srcs = [
            "HwBenchmarkUtils.cpp",
        ],
        headers = [
            "HwBenchmarkUtils.h",
        ],
        versions = to_versions(sai_impl),
        exported_deps = [
            "//fboss/lib:common_utils",
            "//folly:benchmark",
            "//fboss/qsfp_service:core{}".format(core_suffix),
            "//fboss/qsfp_service/platforms/wedge:wedge-platform{}".format(core_suffix),
        ],
    )

def _qsfp_bench_test_lib(impl):
    impl_suffix = to_sdk_suffix(impl)

    # for now, use default SAI impl for core if using native SDK
    core_suffix = impl_suffix if (impl.sai_version != None) else "-default"
    return cpp_library(
        name = "qsfp_bench_test_lib-{}-{}".format(impl.name, impl.version),
        srcs = [
            "PhyInitBenchmark.cpp",
            "ReadWriteRegisterCR4100GBenchmark.cpp",
            "ReadWriteRegisterCWDM4100GBenchmark.cpp",
            "ReadWriteRegisterFR4200GBenchmark.cpp",
            "ReadWriteRegisterFR4400GBenchmark.cpp",
            "ReadWriteRegisterLR4400G10KMBenchmark.cpp",
            "RefreshTcvrCR4100GBenchmark.cpp",
            "RefreshTcvrCWDM4100GBenchmark.cpp",
            "RefreshTcvrFR4200GBenchmark.cpp",
            "RefreshTcvrFR4400GBenchmark.cpp",
            "RefreshTcvrLR4400G10KMBenchmark.cpp",
            "UpdateXphyStatsBenchmark.cpp",
        ],
        link_whole = True,
        undefined_symbols = True,
        versions = to_versions(impl),
        exported_deps = [
            "//folly:benchmark",
            ":hw_bench_utils{}".format(impl_suffix),
            "//fboss/qsfp_service/platforms/wedge:wedge-platform{}".format(core_suffix),
        ],
    )

def _hw_qsfp_bench_test(impl, versions):
    impl_suffix = to_sdk_suffix(impl)

    # for now, use default SAI impl for core if using native SDK
    core_suffix = impl_suffix if (impl.sai_version != None) else "-default"
    return cpp_binary(
        name = "qsfp_bench_test-{}-{}".format(impl.name, impl.version),
        srcs = [
        ],
        auto_headers = AutoHeaders.SOURCES,
        versions = versions,
        deps = [
            "//fboss/lib/test/benchmarks:hw_benchmark_main",
            "//fboss/qsfp_service/platforms/wedge:wedge-platform{}".format(core_suffix),
            "//fboss/qsfp_service/test/benchmarks:qsfp_bench_test_lib-{}-{}".format(impl.name, impl.version),
        ],
    )

def all_hw_bench_utils_libs():
    sai_impls = SAI_PHY_IMPLS
    for sai_impl in sai_impls:
        _hw_bench_utils_lib(sai_impl)
    native_impls = get_all_native_phy_impls()
    for impl in native_impls:
        _hw_bench_utils_lib(impl)

def all_qsfp_bench_test_libs():
    sai_impls = SAI_PHY_IMPLS
    for sai_impl in sai_impls:
        _qsfp_bench_test_lib(sai_impl)
    native_impls = get_all_native_phy_impls()
    for impl in native_impls:
        _qsfp_bench_test_lib(impl)

def all_hw_qsfp_bench_test_binaries():
    sai_impls = SAI_PHY_IMPLS
    for sai_impl in sai_impls:
        _hw_qsfp_bench_test(sai_impl, to_versions(sai_impl))
    native_impls = get_all_native_phy_impls()
    for impl in native_impls:
        _hw_qsfp_bench_test(impl, to_versions(impl))
