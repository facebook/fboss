#
# The fboss project interacts with three third-party libraries
# (broadcom-xgs-robo, bf and opennsl).
# We build against multiple versions of each library, in various combinations.
# See https://fb.quip.com/aaiqAo5OAkia for a complete explanation.
#

load("//fboss/build:sdk.bzl", "get_buildable_sdks")
load("//fboss/build:sdk.thrift.bzl", "ProductLine")

# mapping from sdk_name -> extra version to append to versions map
EXTRA_VERSION_DEPS = {
    "broadcom-plp-barchetta2": {
        "5.2": {
            "broadcom-plp-epdm": "4.1.2",
        },
    },
    "broadcom-plp-millenio": {
        "5.5": {
            "broadcom-plp-epdm": "4.1.2",
        },
    },
}

def to_versions(sdk):
    if sdk.name == "fake":
        return None
    versions = {sdk.sdk_name: sdk.major_version}
    versions.update(EXTRA_VERSION_DEPS.get(sdk.sdk_name, {}).get(sdk.major_version, {}))
    return versions

def to_impl_suffix(sdk):
    if sdk.name == "fake":
        return "-{}".format(sdk.name)
    if sdk.name == "native_bcm":
        # historically our native bcm targets just have the major_version in the name
        return "-{}".format(sdk.major_version)
    else:
        return "-{}-{}".format(
            sdk.name,
            sdk.major_version,
        )

BCM_SDKS = get_buildable_sdks(product_lines = [ProductLine.BCM_NATIVE_SDK])
SAI_FAKE_IMPLS = get_buildable_sdks(product_lines = [ProductLine.FAKE_SDK])
SAI_BRCM_HW_IMPLS = get_buildable_sdks(product_lines = [ProductLine.SAI_SDK_BCM])
SAI_BRCM_SIM_IMPLS = get_buildable_sdks(product_lines = [ProductLine.SAI_SDK_BCM], is_sim = True)
SAI_BRCM_DNX_SIM_IMPLS = get_buildable_sdks(product_lines = [ProductLine.BCM_DSF_SDK], is_sim = True)
SAI_BRCM_DNX_IMPLS = get_buildable_sdks(product_lines = [ProductLine.BCM_DSF_SDK])
SAI_BRCM_IMPLS = SAI_BRCM_HW_IMPLS + SAI_BRCM_SIM_IMPLS + SAI_BRCM_DNX_IMPLS + SAI_BRCM_DNX_SIM_IMPLS
SAI_LEABA_IMPLS = get_buildable_sdks(product_lines = [ProductLine.LEABA])

# NOTE: Before deleting any of these, we must verify that they are not
# referenced in configerator/source/neteng/fboss/fbpkg/qsfp_service.cinc as our
# conveyor attempts to build qsfp_service binaries against each SDK referenced
# in that config
SAI_CREDO_IMPLS = get_buildable_sdks(product_lines = [ProductLine.CREDO_SAI_SDK])

SAI_PHY_IMPLS = SAI_CREDO_IMPLS
SAI_VENDOR_IMPLS = SAI_BRCM_IMPLS + SAI_LEABA_IMPLS + SAI_CREDO_IMPLS

SAI_IMPLS = SAI_VENDOR_IMPLS + SAI_FAKE_IMPLS

# Certain native SDKs have version dependencies. For example, for version 5.2
# of the barchetta2 driver, we need to use version 4.1.2+ of the epdm library,
# otherwise we'll have header mismatches.
# NOTE: Similar to the SAI comment above, any version referenced in
# qsfp_service.cinc must be present here
NATIVE_IMPLS = get_buildable_sdks(product_lines = [ProductLine.MILLENIO_SDK, ProductLine.BARCHETTA2_SDK])
