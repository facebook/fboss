namespace cpp2 facebook.fboss.platform.data_corral_service

enum LedColor {
  OFF = 0,
  GREEN = 1,
  RED = 2,
  BLUE = 3,
  AMBER = 4,
}

// LedConfig holds a mechanism to program LED in the case of presence and absence.
//
// `presentLedColor`: Led color for presence
//
// `presentLedSysfsPath`: Sysfs path for presence
//
// `absentLedColor`: Led color for absence
//
// `absentLedSysfsPath`: Sysfs path for absence.
struct LedConfig {
  1: LedColor presentLedColor;
  2: string presentLedSysfsPath;
  3: LedColor absentLedColor;
  4: string absentLedSysfsPath;
}

// FruConfig contains information to detect FRU presence.
//
// `fruName`: Name of FRU. E.g FAN1, PEM1
//
// `fruType`: Type of FRU. E.g FAN, PEM, PSU
//
// `presenceSysfsPath`: Sysfs  path to detect presence of this FRU.
struct FruConfig {
  1: string fruName;
  2: string fruType;
  3: string presenceSysfsPath;
}

// Main struct to support generic LED programming for presence.
//
// `systemLedConfig`: Led config at system level. System is present if and only if all frus are present.
//
// `fruTypeLedConfigs`: Presence LED programming mechanism for every fru type.
//
// `fruConfigs`: FRU presence detection mechanism in the platform.
struct LedManagerConfig {
  1: LedConfig systemLedConfig;
  2: map<string/*fruType*/ , LedConfig> fruTypeLedConfigs;
  3: list<FruConfig> fruConfigs;
}
