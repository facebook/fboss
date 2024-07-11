namespace cpp2 facebook.fboss
namespace go neteng.fboss.transceiver_validation
namespace php fboss
namespace py neteng.fboss.transceiver_validation
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.transceiver_validation

include "fboss/qsfp_service/if/transceiver.thrift"
include "fboss/lib/if/fboss_common.thrift"
include "fboss/agent/switch_config.thrift"
include "thrift/annotation/cpp.thrift"

struct FirmwarePair {
  1: string applicationFirmwareVersion;
  2: string dspFirmwareVersion;
}

struct TransceiverAttributes {
  1: transceiver.MediaInterfaceCode mediaInterfaceCode;
  2: list<FirmwarePair> supportedFirmwareVersions;
  3: list<switch_config.PortProfileID> supportedPortProfiles;
}

struct VendorConfig {
  1: string vendorName;
  @cpp.Type{
    name = "std::unordered_map<std::string, facebook::fboss::TransceiverAttributes>",
  }
  2: map<string, TransceiverAttributes> partNumberToTransceiverAttributes;
}

struct PlatformTransceiverValidationConfig {
  1: map<fboss_common.PlatformType, list<VendorConfig>> platformToVendorConfigs;
}
