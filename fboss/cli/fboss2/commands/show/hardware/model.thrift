package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowHardwareModel {
  1: list<HWModule> modules;
  2: string ctrlUptime;
  3: string bgpdUptime;
}

struct HWModule {
  1: string moduleName;
  2: string moduleType;
  3: string serialNumber;
  4: string assetTag;
  5: string macAddress;
  6: string fpgaVersion;
  7: string modStatus;
}
