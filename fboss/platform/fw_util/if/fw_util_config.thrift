namespace cpp2 facebook.fboss.platform.fw_util_config

struct FwConfig {
  // Some firmware entity (not all) necessitate a pre upgrade command to be run first
  1: optional string preUpgradeCmd;

  // Command to be used to obtain the version of the firmware entity
  2: string getVersionCmd;

  // some platforms will (not all) have specific priority order in which firmware must be upgraded.
  // The lower the number, the higher the priority is
  3: i32 priority = 0;

  // contains the upgrade command to upgrade the firmware entity.
  4: string upgradeCmd;

  // Some firmware entity necessitate a post upgrade command to be run once the upgrade is complete
  5: optional string postUpgradeCmd;

  // command used to verify that the firmware upgrade went through as expected
  6: string verifyFwCmd;

  // command to read the firmware binary image back into a file
  7: string readFwCmd;
}

typedef string DeviceName

// The configuration for firmware mapping. Each platform will have several
// firmware entity that needs to be upgraded
struct FwUtilConfig {
  1: map<DeviceName, FwConfig> fwConfigs;
}
