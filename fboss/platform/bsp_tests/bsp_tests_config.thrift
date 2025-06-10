// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.bsp_tests

include "fboss/platform/platform_manager/platform_manager_config.thrift"
include "fboss/platform/bsp_tests/fbiob_device_config.thrift"

// Test configuration structure
// testData is mapping of pmName to DeviceTestData
struct BspTestsConfig {
  1: string platform;
  2: map<string, DeviceTestData> testData;
}

struct DeviceTestData {
  1: optional I2CTestData i2cTestData;
  2: optional HwmonTestData hwmonTestData;
  3: optional GpioTestData gpioTestData;
  4: optional WatchdogTestData watchdogTestData;
  5: optional list<LedTestData> ledTestData;
}

struct I2CTestData {
  1: list<I2CDumpData> i2cDumpData;
  2: list<I2CGetData> i2cGetData;
  3: list<I2CSetData> i2cSetData;
}

struct I2CDumpData {
  1: string start;
  2: string end;
  3: list<string> expected;
}

struct I2CGetData {
  1: string reg;
  2: string expected;
}

struct I2CSetData {
  1: string reg;
  2: string value;
}

struct GpioTestData {
  1: i32 numLines;
  2: list<GpioLineInfo> lines;
}

struct GpioLineInfo {
  1: string name;
  2: string direction;
  3: optional i32 getValue;
}

struct HwmonTestData {
  1: list<string> expectedFeatures;
}

struct WatchdogTestData {
  1: i32 numWatchdogs;
}

struct LedTestData {
  1: list<string> expectedColors;
  2: optional string ledType;
  3: optional i32 ledId;
}

// Runtime configuration structure - final configuration used in BspTests
// built by combining PlatformManager config with BspTestConfig data
struct RuntimeConfig {
  1: string platform;
  2: list<PciDevice> devices;
  3: platform_manager_config.BspKmodsFile kmods;
  4: map<string, DeviceTestData> testData;
}

struct PciDevice {
  1: string pmName;
  2: string vendorId;
  3: string deviceId;
  4: string subSystemVendorId;
  5: string subSystemDeviceId;
  6: list<I2CAdapter> i2cAdapters;
  7: list<fbiob_device_config.AuxData> auxDevices;
}

struct I2CAdapter {
  1: string pmName;
  2: fbiob_device_config.AuxData auxDevice;
  3: list<I2CDevice> i2cDevices;
}

struct I2CDevice {
  1: string pmName;
  2: i32 channel;
  3: string deviceName;
  4: string address;
}
