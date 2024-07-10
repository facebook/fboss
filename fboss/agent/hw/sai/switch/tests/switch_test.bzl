load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("@fbcode_macros//build_defs:cpp_unittest.bzl", "cpp_unittest")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_impls_for", "to_impl_lib_name", "to_impl_suffix")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name", "sai_switch_lib_name")

def _manager_test_lib(sai_impl, is_npu):
    switch_lib_name = sai_switch_lib_name(sai_impl, is_npu)
    impl_suffix = to_impl_suffix(sai_impl)
    name = sai_switch_dependent_name("manager_test_base", sai_impl, is_npu)
    return cpp_library(
        name = name,
        srcs = [
            "ManagerTestBase.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        exported_deps = [
            "fbsource//third-party/googletest:gtest",
            "//fboss/agent/hw/sai/fake:fake_sai",
            "//fboss/agent:switchid_scope_resolver",
            "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
            "//fboss/agent/platforms/sai:{}".format(sai_switch_dependent_name("sai_platform", sai_impl, is_npu)),
        ],
    )

def _manager_test_libs(is_npu):
    all_impls = get_all_impls_for(is_npu)
    for sai_impl in all_impls:
        if sai_impl.name != "fake":
            continue
        _manager_test_lib(sai_impl, is_npu)

def manager_test_libs():
    _manager_test_libs(is_npu = True)

def phy_manager_test_libs():
    _manager_test_libs(is_npu = False)

def _switch_manager_unittest_impl(name, srcs, sai_impl, is_npu, **kwargs):
    ut_name = sai_switch_dependent_name(name, sai_impl, is_npu)
    supports_static_listing = kwargs.pop("supports_static_listing", True)
    cpp_unittest(
        name = ut_name,
        srcs = srcs,
        supports_static_listing = supports_static_listing,
        deps = [
            ":{}".format(sai_switch_dependent_name("manager_test_base", sai_impl, is_npu)),
            "//fboss/agent/hw/sai/impl:{}".format(to_impl_lib_name(sai_impl)),
            "//fboss/agent/state:state_utils",
            "//fboss/agent/test:utils",
            "//fboss/mka_service/if:mka_structs-cpp2-types",
        ],
    )

def _switch_manager_unittest(name, srcs, is_npu, **kwargs):
    all_impls = get_all_impls_for(is_npu)
    for sai_impl in all_impls:
        if sai_impl.name != "fake":
            continue
        _switch_manager_unittest_impl(name, srcs, sai_impl, is_npu, **kwargs)

def switch_manager_unittest(name, srcs, **kwargs):
    _switch_manager_unittest(name, srcs, is_npu = True, **kwargs)

def phy_switch_manager_unittest(name, srcs):
    _switch_manager_unittest(name, srcs, is_npu = False)
