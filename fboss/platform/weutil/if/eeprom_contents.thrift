namespace cpp2 facebook.fboss.platform

// This is a common thrift struct for both V5 and V6 EEPROMs
// See https://facebook.github.io/fboss/docs/platform/meta_eeprom_format_v6/ for details.
struct EepromContents {
  1: i16 version;
  2: string productName;
  3: string productPartNumber;
  4: string systemAssemblyPartNumber;
  5: string metaPCBAPartNumber;
  6: string metaPCBPartNumber;
  7: string odmJdmPCBAPartNumber;
  8: string odmJdmPCBASerialNumber;
  9: string productionState;
  10: string productionSubState;
  11: string variantIndicator;
  12: string productSerialNumber;
  13: string systemManufacturer;
  14: string systemManufacturingDate;
  15: string pcbManufacturer;
  16: string assembledAt;
  17: string eepromLocationOnFabric;
  18: string x86CpuMac;
  19: string bmcMac;
  20: string switchAsicMac;
  21: string metaReservedMac;
  22: string rma;
  23: string vendorDefinedField1;
  24: string vendorDefinedField2;
  25: string vendorDefinedField3;
  26: string crc16;
}
