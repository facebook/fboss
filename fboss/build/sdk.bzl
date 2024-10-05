load("//fboss/build:sdk.thrift.bzl", "FbossSdk", "ProductLine", "SdkStage")

# All definitions in this file should be kept in sync with configerator/source/neteng/fboss/fbcode_sdk.cinc

def FakeSdk(version, sai_version):
    return FbossSdk(
        name = "fake",
        product_line = ProductLine.FAKE_SDK,
        product_name = "fake",
        sai_version = sai_version,
        stage = SdkStage.DEV,
        version = version,
    )

def NativeBcmSdk(version, stage):
    return FbossSdk(
        name = "broadcom-xgs-robo",
        is_sai = False,
        product_line = ProductLine.BCM_NATIVE_SDK,
        product_name = "native_bcm",
        sai_version = None,
        stage = stage,
        version = version,
    )

def SaiBrcmSdk(version, sai_version, stage, native_bcm_sdk_version):
    return FbossSdk(
        name = "brcm-sai",
        native_bcm_sdk_version = native_bcm_sdk_version,
        product_line = ProductLine.SAI_SDK_BCM,
        product_name = "brcm",
        sai_version = sai_version,
        stage = stage,
        version = version,
    )

def SaiBrcmSimSdk(version, sai_version):
    return FbossSdk(
        name = "brcm-sai",
        is_sim = True,
        product_line = ProductLine.SAI_SDK_BCM,
        product_name = "brcm",
        sai_version = sai_version,
        stage = SdkStage.TEST,
        version = version,
    )

def SaiBrcmDsfSdk(version, sai_version, stage, fw_path, native_bcm_sdk_version):
    return FbossSdk(
        name = "brcm-sai",
        fw_path = fw_path,
        native_bcm_sdk_version = native_bcm_sdk_version,
        product_line = ProductLine.BCM_DSF_SDK,
        product_name = "brcm",
        sai_version = sai_version,
        stage = stage,
        version = version,
    )

def SaiBrcmDsfSimSdk(version, sai_version):
    return FbossSdk(
        name = "brcm-sai",
        is_sim = True,
        product_line = ProductLine.BCM_DSF_SDK,
        product_name = "brcm",
        sai_version = sai_version,
        stage = SdkStage.TEST,
        version = version,
    )

def SaiLeabaSdk(version, sai_version, stage, fw_path, is_dyn = False):
    return FbossSdk(
        name = "leaba-sdk",
        fw_path = fw_path,
        is_dyn = is_dyn,
        product_line = ProductLine.LEABA,
        product_name = "leaba",
        sai_version = sai_version,
        stage = stage,
        version = version,
    )

def SaiCredoSdk(version, sai_version, stage):
    return FbossSdk(
        name = "CredoB52SAI",
        is_npu = False,
        product_line = ProductLine.CREDO_SAI_SDK,
        product_name = "credo",
        sai_version = sai_version,
        stage = stage,
        version = version,
    )

def NativeMillenioSdk(version, stage):
    return FbossSdk(
        name = "broadcom-plp-millenio",
        is_npu = False,
        is_sai = False,
        product_line = ProductLine.MILLENIO_SDK,
        product_name = "millenio",
        sai_version = None,
        stage = stage,
        version = version,
    )

def NativeBarchettaSdk(version, stage):
    return FbossSdk(
        name = "broadcom-plp-barchetta2",
        is_npu = False,
        is_sai = False,
        product_line = ProductLine.BARCHETTA2_SDK,
        product_name = "barchetta2",
        sai_version = None,
        stage = stage,
        version = version,
    )

def filter(f, arr):
    return [v for v in arr if f(v)]

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

def get_buildable_sdks(product_lines: list, is_sim: bool = False):
    stages = [SdkStage.DEV, SdkStage.TEST, SdkStage.CANARY, SdkStage.COLDBOOT_CANARY, SdkStage.PRODUCTION, SdkStage.PRODUCTION_END]
    return filter_sdks(product_lines = product_lines, stages = stages, is_sim = is_sim)

def get_fbpkg_sdks(product_lines):
    FBPKG_STAGES = [SdkStage.TEST, SdkStage.COLDBOOT_CANARY, SdkStage.CANARY, SdkStage.PRODUCTION, SdkStage.PRODUCTION_END]
    return filter_sdks(is_sim = False, product_lines = product_lines, stages = FBPKG_STAGES)

ALL_SDKS = [
    # Native Bcm
    NativeBcmSdk(
        stage = SdkStage.PRODUCTION,
        version = "6.5.26-1",
    ),
    NativeBcmSdk(
        stage = SdkStage.TEST,
        version = "6.5.28-1",
    ),
    NativeBcmSdk(
        stage = SdkStage.DEV,
        version = "6.5.29-1",
    ),
    NativeBcmSdk(
        stage = SdkStage.NO_STAGE,
        version = "6.5.30-3",
    ),
    # Fake
    FakeSdk(
        sai_version = "1.14.0",
        version = "1.14.0",
    ),
    # Brcm sai
    SaiBrcmSdk(
        native_bcm_sdk_version = "6.5.26",
        sai_version = "1.11.0",
        stage = SdkStage.PRODUCTION,
        version = "8.2.0.0_odp",
    ),
    SaiBrcmSdk(
        native_bcm_sdk_version = "6.5.28",
        sai_version = "1.12.0",
        stage = SdkStage.CANARY,
        version = "9.2.0.0_odp",
    ),
    SaiBrcmSdk(
        native_bcm_sdk_version = "6.5.29",
        sai_version = "1.12.0",
        stage = SdkStage.DEV,
        version = "10.0_ea_odp",
    ),
    SaiBrcmSdk(
        native_bcm_sdk_version = "6.5.29",
        sai_version = "1.13.2",
        stage = SdkStage.COLDBOOT_CANARY,
        version = "10.2.0.0_odp",
    ),
    SaiBrcmSdk(
        native_bcm_sdk_version = "6.5.30",
        sai_version = "1.14.0",
        stage = SdkStage.DEV,
        version = "11.0_ea_odp",
    ),
    SaiBrcmSdk(
        native_bcm_sdk_version = "6.5.30",
        sai_version = "1.14.0",
        stage = SdkStage.DEV,
        version = "11.3.0.0_odp",
    ),
    # Brcm sai sim
    SaiBrcmSimSdk(
        sai_version = "1.11.0",
        version = "8.2.0.0_sim_odp",
    ),
    SaiBrcmSimSdk(
        sai_version = "1.12.0",
        version = "9.0_ea_sim_odp",
    ),
    SaiBrcmSimSdk(
        sai_version = "1.12.0",
        version = "10.0_ea_sim_odp",
    ),
    SaiBrcmDsfSimSdk(
        sai_version = "1.12.0",
        version = "10.0_ea_dnx_sim_odp",
    ),
    # DNX
    SaiBrcmDsfSdk(
        fw_path = "../third-party/tp2/broadcom-xgs-robo/6.5.30_dnx/hsdk-all-6.5.30/tools/sand/db/",
        native_bcm_sdk_version = "6.5.30_dnx",
        sai_version = "1.14.0",
        stage = SdkStage.TEST,
        version = "11.3.0.0_dnx_odp",
    ),
    SaiBrcmDsfSdk(
        fw_path = "../third-party/tp2/broadcom-xgs-robo/6.5.31_dnx/hsdk-all-6.5.31/tools/sand/db/",
        native_bcm_sdk_version = "6.5.31_dnx",
        sai_version = "1.14.0",
        stage = SdkStage.TEST,
        version = "12.0_ea_dnx_odp",
    ),
    # Leaba
    SaiLeabaSdk(
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/1.42.8/res",
        sai_version = "1.7.4",
        stage = SdkStage.PRODUCTION,
        version = "1.42.8",
    ),
    SaiLeabaSdk(
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/24.4.90/res",
        is_dyn = True,
        sai_version = "1.13.0",
        stage = SdkStage.DEV,
        version = "24.4.90",
    ),
    SaiLeabaSdk(
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/24.4.90_yuba/res",
        sai_version = "1.13.0",
        stage = SdkStage.DEV,
        version = "24.4.90_yuba",
    ),
    SaiLeabaSdk(
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/24.6.1_yuba/res",
        sai_version = "1.13.0",
        stage = SdkStage.DEV,
        version = "24.6.1_yuba",
    ),
    SaiLeabaSdk(
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/24.7.0_yuba/res",
        sai_version = "1.13.0",
        stage = SdkStage.DEV,
        version = "24.7.0_yuba",
    ),
    SaiLeabaSdk(
        fw_path = "third-party-buck/platform010-compat/build/leaba-sdk/24.8.3001/res",
        is_dyn = True,
        sai_version = "1.13.0",
        stage = SdkStage.DEV,
        version = "24.8.3001",
    ),
    # Phy
    SaiCredoSdk(
        sai_version = "1.8.1",
        stage = SdkStage.PRODUCTION,
        version = "0.7.2",
    ),
    SaiCredoSdk(
        sai_version = "1.11.0",
        stage = SdkStage.TEST,
        version = "0.8.4",
    ),
    NativeMillenioSdk(
        stage = SdkStage.PRODUCTION,
        version = "5.5",
    ),
    NativeBarchettaSdk(
        stage = SdkStage.PRODUCTION,
        version = "5.2",
    ),
]
