load("@fbsource//tools/build_defs:fb_native_wrapper.bzl", "fb_native")
load("//fboss:THIRD-PARTY-VERSIONS.bzl", "to_impl_suffix")
load("//fboss/agent/fbpkg:common.bzl", "BRCM_SAI_PLATFORM_FMT", "get_agent_sdk_libraries_target")
load("//fboss/build:sdk.bzl", "get_fbpkg_sdks")
load("//fboss/build:sdk.thrift.bzl", "ProductLine")

# Add a target to path actions in the base_dir with the name of the target
# i.e. //fboss/agent/hw/sai/hw_test:sai_test-8.2.0.0_odp will result in
# path_actions[<base_dir>/sai_test-8.2.0.0_odp] = //fboss/agent/hw/sai/hw_test:sai_test-8.2.0.0_odp
def _add_named_target(path_actions, base_dir, target):
    [_, name] = target.split(":")
    path_actions["{}/{}".format(base_dir, name)] = target

# Due to some version universe issues, brcm-sai targets need to be configured with a hardcoded platform
def _brcm_sai_configured_target(target_fmt, sdk):
    [_, test_name_fmt] = target_fmt.split(":")
    configured_test_name = test_name_fmt.format(to_impl_suffix(sdk))
    fb_native.configured_alias(
        name = configured_test_name,
        actual = target_fmt.format(to_impl_suffix(sdk)),
        platform = BRCM_SAI_PLATFORM_FMT.format(brcm_sai_version = sdk.major_version, native_sdk_version = sdk.native_bcm_sdk_version),
        visibility = ["//fboss/agent/fbpkg/..."],
    )
    return ":{}".format(configured_test_name)

# Output path_actions map in the form
# {
#     "sdk-<sdk impl suffix>": {
#         "lib": "<sdk lib target>",
#         "target_name": "target_fmt-<sdk impl suffx>",
#     }
# }
def _produce_standard_test_artifacts_pkg(sdks, bcm_target_fmt, sai_target_fmt):
    path_actions = {}
    for sdk in sdks:
        base_dir = "sdk{}".format(to_impl_suffix(sdk))
        if sdk.product_line == ProductLine.BCM_NATIVE_SDK:
            # some tests don't have the bcm target but still need the libraries
            if bcm_target_fmt:
                target = bcm_target_fmt.format(to_impl_suffix(sdk))
                _add_named_target(path_actions, base_dir, target)
            path_actions["{}/lib".format(base_dir)] = get_agent_sdk_libraries_target(sdk)
        elif sdk.product_line == ProductLine.SAI_SDK_BCM:
            target = _brcm_sai_configured_target(sai_target_fmt, sdk)
            _add_named_target(path_actions, base_dir, target)
            path_actions["{}/lib".format(base_dir)] = get_agent_sdk_libraries_target(sdk)
        elif sdk.product_line == ProductLine.LEABA:
            target = sai_target_fmt.format(to_impl_suffix(sdk))
            _add_named_target(path_actions, base_dir, target)
        else:
            fail("Unsupported sdk for tests in test_artifacts: {}".format(sdk.product_line))
    return path_actions

def get_all_hw_test_path_actions():
    all_sdks = get_fbpkg_sdks([ProductLine.BCM_NATIVE_SDK, ProductLine.SAI_SDK_BCM, ProductLine.LEABA])
    bcm_target_fmt = "//fboss/agent/hw/bcm/tests:bcm_test{}"
    sai_target_fmt = "//fboss/agent/hw/sai/hw_test:sai_test{}"
    return _produce_standard_test_artifacts_pkg(all_sdks, bcm_target_fmt, sai_target_fmt)

def get_all_agent_test_path_actions():
    all_sdks = get_fbpkg_sdks([ProductLine.BCM_NATIVE_SDK, ProductLine.SAI_SDK_BCM, ProductLine.LEABA])
    bcm_target_fmt = "//fboss/agent/test/agent_hw_tests:bcm_agent_hw_test{}"
    sai_target_fmt = "//fboss/agent/test/agent_hw_tests:sai_agent_hw_test{}"
    return _produce_standard_test_artifacts_pkg(all_sdks, bcm_target_fmt, sai_target_fmt)

def get_all_link_test_path_actions():
    all_sdks = get_fbpkg_sdks([ProductLine.BCM_NATIVE_SDK, ProductLine.SAI_SDK_BCM, ProductLine.LEABA])
    bcm_target_fmt = "//fboss/agent/test/link_tests:bcm_link_test{}"
    sai_target_fmt = "//fboss/agent/test/link_tests:sai_link_test{}"
    return _produce_standard_test_artifacts_pkg(all_sdks, bcm_target_fmt, sai_target_fmt)

def get_all_fsdb_integration_test_path_actions():
    all_sdks = get_fbpkg_sdks([ProductLine.BCM_NATIVE_SDK, ProductLine.SAI_SDK_BCM, ProductLine.LEABA])
    bcm_target_fmt = "//fboss/agent/test/fsdb_integration_tests/facebook:bcm_fsdb_test{}"
    sai_target_fmt = "//fboss/agent/test/fsdb_integration_tests/facebook:sai_fsdb_integration_test{}"
    return _produce_standard_test_artifacts_pkg(all_sdks, bcm_target_fmt, sai_target_fmt)

def get_all_invariant_test_path_actions():
    all_sdks = get_fbpkg_sdks([ProductLine.BCM_NATIVE_SDK, ProductLine.SAI_SDK_BCM, ProductLine.LEABA])

    # We don't include invariant test for bcm native
    bcm_target_fmt = None
    sai_target_fmt = "//fboss/agent/test/prod_invariant_tests:sai_invariant_agent_test{}"
    return _produce_standard_test_artifacts_pkg(all_sdks, bcm_target_fmt, sai_target_fmt)
