namespace cpp2 facebook.fboss.platform.showtech_config

// Configuration for the showtech utility
struct ShowtechConfig {
  1: set<string> i2cBusIgnore;
  2: list<PsuConfig> psus;
}

struct PsuConfig {
  1: i16 psuSlot;
  2: ScdI2cDeviceConfig scdI2cDeviceConfig;
}

struct ScdI2cDeviceConfig {
  1: string scdPciAddr;
  2: string deviceAddr;
  3: i16 master;
  4: i16 bus;
}
