load("//fboss/build:sdk.thrift.bzl", "FbossSdk", "ProductLine", "SdkStage")

# All definitions in this file should be kept in sync with configerator/source/neteng/fboss/fbcode_sdk.cinc

def FakeSdk(version, sai_version):
    return FbossSdk(
        product_name = "fake",
        product_line = ProductLine.FAKE_SDK,
        name = "fake",
        version = version,
        sai_version = sai_version,
        stage = SdkStage.DEV,
    )

def NativeBcmSdk(version, stage):
    return FbossSdk(
        product_name = "native_bcm",
        product_line = ProductLine.BCM_NATIVE_SDK,
        name = "broadcom-xgs-robo",
        version = version,
        sai_version = None,
        stage = stage,
        is_sai = False,
    )

def SaiBrcmSdk(version, sai_version, stage, native_bcm_sdk_version):
    return FbossSdk(
        product_name = "brcm",
        product_line = ProductLine.SAI_SDK_BCM,
        name = "brcm-sai",
        version = version,
        sai_version = sai_version,
        stage = stage,
        native_bcm_sdk_version = native_bcm_sdk_version,
    )

def SaiBrcmSimSdk(version, sai_version):
    return FbossSdk(
        product_name = "brcm",
        product_line = ProductLine.SAI_SDK_BCM,
        name = "brcm-sai",
        version = version,
        sai_version = sai_version,
        stage = SdkStage.TEST,
        is_sim = True,
    )

def SaiBrcmDsfSdk(version, sai_version, stage, fw_path, native_bcm_sdk_version):
    return FbossSdk(
        product_name = "brcm",
        product_line = ProductLine.BCM_DSF_SDK,
        name = "brcm-sai",
        version = version,
        sai_version = sai_version,
        stage = stage,
        fw_path = fw_path,
        native_bcm_sdk_version = native_bcm_sdk_version,
    )

def SaiBrcmDsfSimSdk(version, sai_version):
    return FbossSdk(
        product_name = "brcm",
        product_line = ProductLine.BCM_DSF_SDK,
        name = "brcm-sai",
        version = version,
        sai_version = sai_version,
        stage = SdkStage.TEST,
        is_sim = True,
    )

def SaiLeabaSdk(version, sai_version, stage, fw_path):
    return FbossSdk(
        product_name = "leaba",
        product_line = ProductLine.LEABA,
        name = "leaba-sdk",
        version = version,
        sai_version = sai_version,
        stage = stage,
        fw_path = fw_path,
    )

def SaiCredoSdk(version, sai_version, stage):
    return FbossSdk(
        product_name = "credo",
        product_line = ProductLine.CREDO_SAI_SDK,
        name = "CredoB52SAI",
        version = version,
        sai_version = sai_version,
        stage = stage,
        is_npu = False,
    )

def SaiMrvlSdk(version, sai_version, stage):
    return FbossSdk(
        product_name = "mrvl",
        product_line = ProductLine.MRVL_SAI_SDK,
        name = "mrvl_phy_sai",
        version = version,
        sai_version = sai_version,
        stage = stage,
        is_npu = False,
    )

def NativeMillenioSdk(version, stage):
    return FbossSdk(
        product_name = "millenio",
        product_line = ProductLine.MILLENIO_SDK,
        name = "broadcom-plp-millenio",
        version = version,
        sai_version = None,
        stage = stage,
        is_sai = False,
        is_npu = False,
    )

def NativeBarchettaSdk(version, stage):
    return FbossSdk(
        product_name = "barchetta2",
        product_line = ProductLine.BARCHETTA2_SDK,
        name = "broadcom-plp-barchetta2",
        version = version,
        sai_version = None,
        stage = stage,
        is_sai = False,
        is_npu = False,
    )

def filter_sdks_by_name(sdks: list, names: list[str]) -> list:
    return filter(lambda sdk: sdk.name in names, sdks)

def filter_sdks_by_stage(sdks: list, stages: list) -> list:
    return filter(lambda sdk: sdk.stage in stages, sdks)

def filter_sdks_by_product_line(sdks: list, product_lines: list) -> list:
    return filter(lambda sdk: sdk.product_line in product_lines, sdks)

def filter_sdks_by_sim(sdks: list, is_sim: bool) -> list:
    return filter(lambda sdk: sdk.is_sim == is_sim, sdks)

def filter_sdks(
        names: list[str] | None = None,
        stages: list | None = None,
        product_lines: list | None = None,
        is_sim: bool = False) -> list:
    sdks = ALL_SDKS
    if names:
        sdks = filter_sdks_by_name(sdks, names)
    if product_lines:
        sdks = filter_sdks_by_product_line(sdks, product_lines)
    if stages:
        sdks = filter_sdks_by_stage(sdks, stages)

    # Majority of the time we want to filter sim sdks out, so default to that
    sdks = filter_sdks_by_sim(sdks, is_sim)
    return list(sdks)

def get_fbpkg_sdks(product_lines):
    FBPKG_STAGES = [SdkStage.TEST, SdkStage.CANARY, SdkStage.PRODUCTION, SdkStage.PRODUCTION_END]
    return filter_sdks(product_lines = product_lines, stages = FBPKG_STAGES, is_sim = False)

ALL_SDKS = [
    NativeBcmSdk(version = "6.5.26-1", stage = SdkStage.PRODUCTION),
    NativeBcmSdk(version = "6.5.28-1", stage = SdkStage.TEST),
    NativeBcmSdk(version = "6.5.29-1", stage = SdkStage.DEV),
    FakeSdk(version = "1.14.0", sai_version = "1.14.0"),
    SaiBrcmSdk(version = "8.2.0.0_odp", sai_version = "1.11.0", native_bcm_sdk_version = "6.5.26", stage = SdkStage.PRODUCTION),
    SaiBrcmSdk(version = "9.2.0.0_odp", sai_version = "1.12.0", native_bcm_sdk_version = "6.5.28", stage = SdkStage.CANARY),
    SaiBrcmSdk(version = "10.0_ea_odp", sai_version = "1.12.0", native_bcm_sdk_version = "6.5.29", stage = SdkStage.DEV),
    SaiBrcmSdk(version = "10.2.0.0_odp", sai_version = "1.13.2", native_bcm_sdk_version = "6.5.29", stage = SdkStage.DEV),
    SaiBrcmSdk(version = "11.0_ea_odp", sai_version = "1.14.0", native_bcm_sdk_version = "6.5.30", stage = SdkStage.DEV),
    SaiBrcmSimSdk(version = "8.2.0.0_sim_odp", sai_version = "1.11.0"),
    SaiBrcmSimSdk(version = "9.0_ea_sim_odp", sai_version = "1.12.0"),
    SaiBrcmSimSdk(version = "10.0_ea_sim_odp", sai_version = "1.12.0"),
    SaiBrcmDsfSimSdk(version = "10.0_ea_dnx_sim_odp", sai_version = "1.12.0"),
    SaiBrcmDsfSdk(
        version = "11.0_ea_dnx_odp",
        sai_version = "1.13.1",
        stage = SdkStage.TEST,
        fw_path = "../third-party/tp2/broadcom-xgs-robo/6.5.30_dnx/hsdk-all-6.5.30/tools/sand/db/",
        native_bcm_sdk_version = "6.5.30_dnx",
    ),
    SaiBrcmDsfSdk(
        version = "12.0_ea_dnx_odp",
        sai_version = "1.14.0",
        stage = SdkStage.TEST,
        fw_path = "../third-party/tp2/broadcom-xgs-robo/6.5.31_dnx/hsdk-all-6.5.31/tools/sand/db/",
        native_bcm_sdk_version = "6.5.31_dnx",
    ),
    SaiLeabaSdk(
        version = "1.42.8",
        sai_version = "1.7.4",
        stage = SdkStage.PRODUCTION,
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/1.42.8/res",
    ),
    SaiLeabaSdk(
        version = "1.65.1",
        sai_version = "1.13.0",
        stage = SdkStage.CANARY,
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/1.65.1/res",
    ),
    SaiLeabaSdk(
        version = "24.4.90",
        sai_version = "1.13.0",
        stage = SdkStage.DEV,
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/24.4.90/res",
    ),
    SaiLeabaSdk(
        version = "24.4.90_yuba",
        sai_version = "1.13.0",
        stage = SdkStage.DEV,
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/24.4.90_yuba/res",
    ),
    SaiLeabaSdk(
        version = "24.6.1_yuba",
        sai_version = "1.13.0",
        stage = SdkStage.DEV,
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/24.6.1_yuba/res",
    ),
    SaiCredoSdk(version = "0.7.2", sai_version = "1.8.1", stage = SdkStage.PRODUCTION),
    SaiMrvlSdk(version = "1.4.0", sai_version = "1.11.0", stage = SdkStage.DEV),
    NativeMillenioSdk(version = "5.5", stage = SdkStage.PRODUCTION),
    NativeBarchettaSdk(version = "5.2", stage = SdkStage.PRODUCTION),
]
