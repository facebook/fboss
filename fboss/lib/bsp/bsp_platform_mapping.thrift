namespace cpp2 facebook.fboss
namespace py neteng.fboss.bsp_platform_mapping

include 'fboss/lib/led/led_mapping.thrift'

struct BspPlatformMappingThrift {
  1: map<i32, BspPimMapping> pimMapping;
}

struct BspPimMapping {
  1: i32 pimID;
  2: map<i32, BspTransceiverMapping> tcvrMapping;
  3: map<i32, BspPhyMapping> phyMapping;
  // Map of controller ID to controller info
  4: map<i32, BspPhyIOControllerInfo> phyIOControllers;
  5: map<i32, led_mapping.LedMapping> ledMapping;
}

struct BspPhyMapping {
  1: i32 phyId; // unique ID across the system
  2: i32 phyIOControllerId;
  3: i32 phyAddr; // 5 bit phy address
}

struct BspPhyIOControllerInfo {
  1: i32 controllerId; // unique ID for controllers on a PIM
  2: PhyIOType type;
  3: string devicePath; // file path for io access
}

enum PhyIOType {
  UNKNOWN = 0,
  MDIO = 1,
}

struct BspTransceiverMapping {
  1: i32 tcvrId;
  2: BspTransceiverAccessControllerInfo accessControl;
  3: BspTransceiverIOControllerInfo io;
  4: map<i32, i32> tcvrLaneToLedId;
}

struct BspTransceiverAccessControllerInfo {
  1: string controllerId;
  2: ResetAndPresenceAccessType type;
  3: BspResetPinInfo reset;
  4: BspPresencePinInfo presence;
  5: optional string gpioChip;
}

struct BspTransceiverIOControllerInfo {
  1: string controllerId;
  2: TransceiverIOType type;
  3: string devicePath;
}

enum TransceiverIOType {
  UNKNOWN = 0,
  I2C = 1,
}

struct BspResetPinInfo {
  1: optional string sysfsPath;
  2: optional i32 mask;
  3: optional i32 gpioOffset;
}

struct BspPresencePinInfo {
  1: optional string sysfsPath;
  2: optional i32 mask;
  3: optional i32 gpioOffset;
}

enum ResetAndPresenceAccessType {
  UNKNOWN = 0,
  CPLD = 1,
  GPIO = 2,
}
