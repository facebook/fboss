load("@fbsource//tools/build_defs:fb_native_wrapper.bzl", "fb_native")
load("//fboss:THIRD-PARTY-VERSIONS.bzl", "to_impl_suffix")
load("//fboss/agent/fbpkg:common.bzl", "BRCM_SAI_PLATFORM_FMT", "get_agent_sdk_libraries_target")
load("//fboss/build:sdk.bzl", "get_fbpkg_sdks")
load("//fboss/build:sdk.thrift.bzl", "ProductLine")

_SAI_BCM_AGENT_ALIAS_PREFIX = "brcm_sai_wedge_agent"
_SAI_BCM_HW_AGENT_ALIAS_PREFIX = "brcm_sai_hw_agent"

def _get_agent_target(sdk):
    if sdk.product_line == ProductLine.BCM_NATIVE_SDK:
        return "//fboss/agent/platforms/wedge:wedge_agent_{}".format(sdk.major_version)
    elif sdk.product_line in (ProductLine.SAI_SDK_BCM, ProductLine.BCM_DSF_SDK):
        return ":{}{}".format(_SAI_BCM_AGENT_ALIAS_PREFIX, to_impl_suffix(sdk))
    elif sdk.product_line in (ProductLine.BCM_DSF_SDK, ProductLine.LEABA):
        return "//fboss/agent/platforms/sai:wedge_agent{}".format(to_impl_suffix(sdk))
    else:
        fail("Unsupported product line {}".format(sdk.product_line))

def _get_hw_agent_target(sdk):
    if sdk.product_line in (ProductLine.SAI_SDK_BCM, ProductLine.BCM_DSF_SDK):
        return ":{}{}".format(_SAI_BCM_HW_AGENT_ALIAS_PREFIX, to_impl_suffix(sdk))
    elif sdk.product_line == ProductLine.LEABA:
        return "//fboss/agent/platforms/sai:fboss_hw_agent{}".format(to_impl_suffix(sdk))
    else:
        fail("Unsupported product line {}".format(sdk.product_line))

def _get_wedge_agent_path_actions(sdks, include_split_bins = True):
    actions = {}

    for sdk in sdks:
        base_dir = "sdk-{}".format(sdk.major_version)
        actions[base_dir] = {}
        actions[base_dir]["wedge_agent"] = _get_agent_target(sdk)
        actions[base_dir]["lib"] = get_agent_sdk_libraries_target(sdk)
        if include_split_bins:
            actions[base_dir]["fboss_hw_agent"] = _get_hw_agent_target(sdk)
    return actions

def get_scripts_path_actions():
    return {
        "agent_executor_runner": "//fboss/agent/facebook:agent_executor_runner",
        "agent_pre_start_exec_runner": "//fboss/agent:agent_pre_start_exec_runner",
        "fboss_sw_agent": "//fboss/agent:fboss_sw_agent",
        "pre_wedge_agent_shut_runner.par": "//neteng/fboss/tools/wrapper_scripts:pre_wedge_agent_shut_runner",
        "wedge_agent.service": "//neteng/fboss/scripts:wedge_agent.service",
        "wedge_agent.sh": "//neteng/fboss/scripts:wedge_agent.sh",
        "wedge_agent_wrapper.par": "//neteng/fboss/tools/wrapper_scripts:wedge_agent_wrapper",
    }

def _gen_brcm_sai_constraint_aliases(sdks):
    for sdk in sdks:
        fb_native.configured_alias(
            name = "{}{}".format(_SAI_BCM_AGENT_ALIAS_PREFIX, to_impl_suffix(sdk)),
            actual = "//fboss/agent/platforms/sai:wedge_agent{}".format(to_impl_suffix(sdk)),
            platform = BRCM_SAI_PLATFORM_FMT.format(brcm_sai_version = sdk.major_version, native_sdk_version = sdk.native_bcm_sdk_version),
            visibility = ["//fboss/agent/fbpkg/...", "//netos/..."],
        )
        fb_native.configured_alias(
            name = "{}{}".format(_SAI_BCM_HW_AGENT_ALIAS_PREFIX, to_impl_suffix(sdk)),
            actual = "//fboss/agent/platforms/sai:fboss_hw_agent{}".format(to_impl_suffix(sdk)),
            platform = BRCM_SAI_PLATFORM_FMT.format(brcm_sai_version = sdk.major_version, native_sdk_version = sdk.native_bcm_sdk_version),
            visibility = ["//fboss/agent/fbpkg/...", "//netos/..."],
        )

def _gen_dnx_sai_constraint_aliases(sdks):
    for sdk in sdks:
        fb_native.configured_alias(
            name = "{}{}".format(_SAI_BCM_AGENT_ALIAS_PREFIX, to_impl_suffix(sdk)),
            actual = "//fboss/agent/platforms/sai:wedge_agent{}".format(to_impl_suffix(sdk)),
            platform = BRCM_SAI_PLATFORM_FMT.format(brcm_sai_version = sdk.major_version, native_sdk_version = sdk.native_bcm_sdk_version),
            visibility = ["//fboss/agent/fbpkg/...", "//netos/..."],
        )
        fb_native.configured_alias(
            name = "{}{}".format(_SAI_BCM_HW_AGENT_ALIAS_PREFIX, to_impl_suffix(sdk)),
            actual = "//fboss/agent/platforms/sai:fboss_hw_agent{}".format(to_impl_suffix(sdk)),
            platform = BRCM_SAI_PLATFORM_FMT.format(brcm_sai_version = sdk.major_version, native_sdk_version = sdk.native_bcm_sdk_version),
            visibility = ["//fboss/agent/fbpkg/...", "//netos/..."],
        )

def get_all_wedge_agent_brcm_native_path_actions():
    bcm_sdks = get_fbpkg_sdks([ProductLine.BCM_NATIVE_SDK])
    path_actions = {}
    path_actions.update(_get_wedge_agent_path_actions(bcm_sdks, include_split_bins = False))
    return path_actions

def get_all_wedge_agent_brcm_sai_path_actions(flatten_path_actions = True):
    sai_sdks = get_fbpkg_sdks([ProductLine.SAI_SDK_BCM])
    _gen_brcm_sai_constraint_aliases(sai_sdks)

    path_actions = _get_wedge_agent_path_actions(sai_sdks)
    if flatten_path_actions:
        path_actions = _flatten_path_actions(path_actions)
    return path_actions

def _flatten_path_actions(current_actions):
    path_actions = {}
    for target, target_info in current_actions.items():
        for target_name, target_value in target_info.items():
            path_actions["{}/{}".format(target, target_name)] = target_value
    return path_actions

def get_all_wedge_agent_path_actions(include_scripts = True, flatten_path_actions = True):
    sai_path_actions = get_all_wedge_agent_brcm_sai_path_actions(flatten_path_actions = False)
    native_path_actions = get_all_wedge_agent_brcm_native_path_actions()
    native_path_actions.update(sai_path_actions)
    path_actions = native_path_actions
    if flatten_path_actions:
        path_actions = _flatten_path_actions(native_path_actions)
    if include_scripts:
        path_actions.update(get_scripts_path_actions())
    return path_actions

def get_all_wedge_agent_csco_path_actions(flatten_path_actions = True, include_scripts = True):
    csco_sdks = get_fbpkg_sdks([ProductLine.LEABA])

    path_actions = {}
    path_actions.update(_get_wedge_agent_path_actions(csco_sdks))
    if flatten_path_actions:
        path_actions = _flatten_path_actions(path_actions)
    if include_scripts:
        path_actions.update(get_scripts_path_actions())
    return path_actions

def get_all_wedge_agent_dnx_path_actions(flatten_path_actions = True, include_scripts = True):
    dnx_sdks = get_fbpkg_sdks([ProductLine.BCM_DSF_SDK])
    _gen_dnx_sai_constraint_aliases(dnx_sdks)

    path_actions = {}
    path_actions.update(_get_wedge_agent_path_actions(dnx_sdks))
    if flatten_path_actions:
        path_actions = _flatten_path_actions(path_actions)
    if include_scripts:
        path_actions.update(get_scripts_path_actions())
    return path_actions
