load("//fboss/build:sdk.thrift.bzl", "ProductLine")

BRCM_SAI_PLATFORM_FMT = "ovr_config//platform/linux:x86_64-fbcode-platform010-compat-clang15-FBOSS-projects-brcm-sai-{brcm_sai_version}-brcm-sai-{brcm_sai_version}-broadcom-xgs-robo-{native_sdk_version}"

def get_agent_sdk_libraries_target(sdk):
    if sdk.product_line == ProductLine.BCM_NATIVE_SDK:
        return "//third-party-buck/platform010-compat/build/broadcom-xgs-robo/{}:libraries".format(sdk.major_version)
    elif sdk.product_line == ProductLine.BCM_DSF_SDK:
        return "//third-party-buck/platform010-compat/build/broadcom-xgs-robo/{}:libraries".format(sdk.native_bcm_sdk_version)
    elif sdk.product_line == ProductLine.SAI_SDK_BCM:
        return "//third-party-buck/platform010-compat/build/brcm-sai/{}:libraries".format(sdk.major_version)
    elif sdk.product_line == ProductLine.LEABA:
        return "//third-party-buck/platform010-compat/build/leaba-sdk/{}:libraries".format(sdk.major_version)
    else:
        fail("Unsupported product line {}".format(sdk.product_line))
