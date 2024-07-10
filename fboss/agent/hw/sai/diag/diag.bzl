load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_impls", "get_all_npu_impls", "get_all_phy_impls", "to_impl_suffix", "to_versions")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name", "sai_switch_lib_name")

def _py_repl_lib(sai_impl):
    impl_suffix = to_impl_suffix(sai_impl)
    subdir = "oss"
    if sai_impl.name == "leaba":
        subdir = "facebook/tajo"

    cpp_library(
        name = "python_repl{}".format(impl_suffix),
        srcs = [
            "PythonRepl.cpp",
            "{}/PythonRepl.cpp".format(subdir),
        ],
        auto_headers = AutoHeaders.SOURCES,
        exported_deps = [
            ":diag_lib",
            "//folly:file",
            "//folly:format",
            "//folly/logging:logging",
        ],
        exported_external_deps = [
            ("python", None, "python"),
        ],
    )

def _diag_lib(sai_impl, is_npu):
    impl_suffix = to_impl_suffix(sai_impl)
    switch_lib_name = sai_switch_lib_name(sai_impl, is_npu)
    return cpp_library(
        name = "{}".format(sai_switch_dependent_name("diag_shell", sai_impl, is_npu)),
        srcs = [
            "DiagShell.cpp",
            "facebook/DiagShell.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
        exported_deps = [
            "//fboss/agent/hw/sai/switch:if-cpp2-services",
            "//fboss/agent/hw/sai/switch:if-cpp2-types",
            ":sai_repl{}".format(impl_suffix),
            ":python_repl{}".format(impl_suffix),
            "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
            "//folly:file",
            "//folly:file_util",
            "//folly/logging:logging",
            "//thrift/lib/cpp2/async:server_stream",
        ],
        exported_external_deps = [("boost", None, "boost_uuid")],
    )

def _sai_repl(sai_impl):
    impl_suffix = to_impl_suffix(sai_impl)
    return cpp_library(
        name = "sai_repl{}".format(impl_suffix),
        srcs = [
            "SaiRepl.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
        exported_deps = [
            ":diag_lib",
            "//fboss/agent/hw/sai/api:sai_api{}".format(impl_suffix),
            "//folly:file",
            "//folly/logging:logging",
        ],
    )

def all_diag_libs():
    for sai_impl in get_all_impls():
        _py_repl_lib(sai_impl)

    for sai_impl in get_all_npu_impls():
        _diag_lib(sai_impl, True)

    for sai_impl in get_all_phy_impls():
        _diag_lib(sai_impl, False)

def all_repl_libs():
    all_impls = get_all_impls()
    for sai_impl in all_impls:
        _sai_repl(sai_impl)

def _diag_shell_client_bins():
    return cpp_binary(
        name = "diag_shell_client",
        srcs = [
            "DiagShellClient.cpp",
            "facebook/DiagShellClient.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        deps = [
            "//fboss/agent/hw/sai/switch:if-cpp2-clients",
            "//folly/init:init",
            "//folly/io/async:async_base",
            "//folly/io/async:async_signal_handler",
            "//folly/logging:init",
            "//folly/logging:logging",
            "//servicerouter/client/cpp2:cpp2",
        ],
    )

def all_diag_bins():
    _diag_shell_client_bins()
