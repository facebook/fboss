namespace cpp2 facebook.fboss
namespace py neteng.fboss.bsp_platform_mapping

struct BspPlatformMappingThrift {
  1: map<i32, BspPimMapping> pimMapping;
}

struct BspPimMapping {
  1: i32 pimID;
  2: map<i32, BspTransceiverMapping> tcvrMapping;
// TODO: Add LEDs, PHY mapping
}

struct BspTransceiverMapping {
  1: i32 tcvrId;
  2: BspTransceiverAccessControllerInfo accessControl;
  3: BspTransceiverIOControllerInfo io;
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
