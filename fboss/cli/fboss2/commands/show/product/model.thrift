namespace cpp2 facebook.fboss.cli

struct ShowProductModel {
  1: string product;
  2: string oem;
  3: string serial;
}

struct ShowProductDetailsModel {
  1: string product;
  2: string oem;
  3: string serial;
  4: string mgmtMac;
  5: string bmcMac;
  6: string macRangeStart;
  7: string macRangeSize;
  8: string assembledAt;
  9: string assetTag;
  10: string partNumber;
  11: string productionState;
  12: string subVersion;
  13: string productVersion;
  14: string systemPartNumber;
  15: string mfgDate;
  16: string pcbManufacturer;
  17: string fbPcbaPartNumber;
  18: string fbPcbPartNumber;
  19: string odmPcbaPartNumber;
  20: string odmPcbaSerial;
  21: string version;
}
