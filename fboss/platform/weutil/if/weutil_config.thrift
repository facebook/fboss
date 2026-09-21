include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace cpp2 facebook.fboss.platform.weutil_config

typedef string FruEepromPath
typedef string FruEepromName
typedef i32 FruIdOffset

struct FruEepromConfig {
  // the unique path of the FRU ID EEPROM
  // e.g. "/run/devmap/eeproms/FANSPINNER_EEPROM"
  1: FruEepromPath path;
  // FRU ID section offset in the EEPROM, default is 0
  2: FruIdOffset offset = 0;
}

struct WeutilConfig {
  // ID EEPROM for Chassis, when executing weutil without EEPROM parameter
  // It may have different FRU based names based on different platforms/design
  1: FruEepromName chassisEepromName = "CHASSIS";
  2: map<FruEepromName, FruEepromConfig> fruEepromList;
}
