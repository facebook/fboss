# Mirror of defs in configerator/source/neteng/fboss/thrift/sdk.thrift
SdkStage = struct(
    NO_STAGE = 0,
    DEV = 1,
    TEST = 2,
    CANARY = 3,
    PRODUCTION = 4,
    PRODUCTION_END = 5,
    COLDBOOT_CANARY = 6,
)

ProductLine = struct(
    PRODUCT_LINE_NONE = 0,
    BCM_NATIVE_SDK = 1,
    BCM_SDK_FOR_SAI = 2,  # The BCM SDK to support SAI,
    SAI_SDK_BCM = 3,  # The SAI SDK from Broadcom
    LEABA = 4,  # The Leaba SDK from Cisco
    BCM_NATIVE_SDK_TH4 = 5,  # The Bcm SDK for TH4
    LEABA_DSF_SDK = 6,  # DSF SDK from Cisco
    BCM_DSF_SDK = 7,  # DSF SDK Bcm
    CREDO_SAI_SDK = 8,  # Credo SAI SDK
    MILLENIO_SDK = 9,  # Millenio native SDK
    BARCHETTA2_SDK = 10,  # Barchetta2 native SDK
    FAKE_SDK = 11,  # Fake SDK
    MRVL_SAI_SDK = 12,
)

def FbossSdk(
        product_name,
        product_line,
        name,
        version,
        sai_version,
        stage,
        is_npu = True,
        is_sai = True,
        is_sim = False,
        is_dyn = False,
        fw_path = None,
        native_bcm_sdk_version = None):
    return struct(
        # name in sdk.cconf is the sdk_name, so renaming fields here for now
        name = product_name,
        sdk_name = name,
        version = version,
        sai_version = sai_version,
        is_sai = is_sai,
        is_npu = is_npu,
        stage = stage,
        is_sim = is_sim,
        is_dyn = is_dyn,
        product_line = product_line,
        fw_path = fw_path,
        native_bcm_sdk_version = native_bcm_sdk_version,

        # fields only used in fbcode and not mirrored in thrift
        major_version = version.split("-")[0],
    )
