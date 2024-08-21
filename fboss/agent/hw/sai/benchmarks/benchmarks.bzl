load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_npu_impls", "to_impl_lib_name", "to_impl_suffix", "to_versions")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name")

def _mono_sai_agent_benchmark_binary(name, srcs, sai_impl, **kwargs):
    impl_suffix = to_impl_suffix(sai_impl)
    name = "{}-{}-{}".format(name, sai_impl.name, sai_impl.version)
    main_name = "mono_sai_agent_benchmarks_main"
    sai_agent_benchmarks_main = sai_switch_dependent_name(main_name, sai_impl, True)
    agent_hw_test_thrift_handler = sai_switch_dependent_name("agent_hw_test_thrift_handler", sai_impl, True)
    deps = list(kwargs.get("deps", []))
    deps += [
        "//fboss/agent/hw/sai/impl:{}".format(to_impl_lib_name(sai_impl)),
        "//fboss/agent:main-sai-{}".format(to_impl_lib_name(sai_impl)),
        "//fboss/agent/hw/sai/hw_test:sai_acl_utils{}".format(impl_suffix),
        "//fboss/agent/hw/sai/hw_test:sai_copp_utils{}".format(impl_suffix),
        "//fboss/agent/hw/sai/hw_test:sai_ecmp_utils{}".format(impl_suffix),
        "//fboss/agent/hw/sai/hw_test:sai_packet_trap_helper{}".format(impl_suffix),
        "//fboss/agent/hw/sai/hw_test:sai_port_utils{}".format(impl_suffix),
        "//fboss/agent/benchmarks:{}".format(sai_agent_benchmarks_main),
        "//fboss/agent/hw/sai/hw_test:{}".format(agent_hw_test_thrift_handler),
        "//fboss/agent/test:linkstate_toggler",
    ]
    kwargs["deps"] = deps
    return cpp_binary(
        name = name,
        srcs = srcs,
        versions = to_versions(sai_impl),
        **kwargs
    )

def _multi_switch_agent_benchmark_binary(name, srcs, **kwargs):
    name = "multi_switch-{}".format(name)
    main_name = "multi_switch_sai_agent_benchmarks_main"
    sai_agent_benchmarks_main = main_name
    deps = list(kwargs.get("deps", []))
    deps += [
        "//fboss/agent/benchmarks:{}".format(sai_agent_benchmarks_main),
        "//fboss/agent/test:linkstate_toggler",
    ]
    kwargs["deps"] = deps
    return cpp_binary(
        name = name,
        srcs = srcs,
        **kwargs
    )

def sai_mono_agent_benchmark(name, srcs, **kwargs):
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        if sai_impl.name == "fake":
            continue
        _mono_sai_agent_benchmark_binary(name, srcs, sai_impl, **kwargs)

def sai_multi_switch_agent_benchmark(name, srcs, **kwargs):
    _multi_switch_agent_benchmark_binary(name, srcs, **kwargs)
