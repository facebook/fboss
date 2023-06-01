namespace cpp2 facebook.fboss.platform.weutil_config

typedef string FruEepromPath
typedef string FruEepromName
typedef i32 FruIdOffset

enum IdEepromFormatVersion {
  // Reserved for prefdl, backwards compatibility
  V0 = 0,
  // Meta ID Eeprom Format Version 3
  V3 = 3,
  // Meta ID Eeprom Format Version 4
  V4 = 4,
  // Meta ID Eeprom Format Version 5
  V5 = 5,
}

struct FruEepromConfig {
  // the unique path of the FRU ID EEPROM
  // e.g. "/run/devmap/eeproms/FANSPINNER_EEPROM"
  1: FruEepromPath path;
  // FRU ID section offset in the EEPROM, default is 0
  2: FruIdOffset offset = 0;
  // ID EEPROM format version
  3: IdEepromFormatVersion idEepromFormatVer = V4;
}

struct WeutilConfig {
  // ID EEPROM for Chassis, when executing weutil without EEPROM parameter
  // It may have different FRU based names based on different platforms/design
  1: FruEepromName chassisEepromName = "CHASSIS";
  2: map<FruEepromName, FruEepromConfig> fruEepromList;
}
