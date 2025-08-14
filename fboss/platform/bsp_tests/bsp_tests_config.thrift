// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.bsp_tests
include "thrift/annotation/thrift.thrift"

include "fboss/platform/platform_manager/platform_manager_config.thrift"
include "fboss/platform/bsp_tests/fbiob_device_config.thrift"

// Test configuration structure
// testData is mapping of pmName to DeviceTestData
struct BspTestsConfig {
  1: map<string, DeviceTestData> testData;
  2: map<string, map<ExpectedErrorType, string>> expectedErrors; //pmName : {errorType: reason}
}
enum ExpectedErrorType {
  UNKNOWN_ERROR = 0,
  I2C_NOT_DETECTABLE = 1,
}

struct DeviceTestData {
  1: optional I2CTestData i2cTestData;
  2: optional HwmonTestData hwmonTestData;
  3: optional GpioTestData gpioTestData;
  4: optional WatchdogTestData watchdogTestData;
  5: optional LedTestData ledTestData;
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
  1: i32 index;
  2: string name;
  3: string direction;
  4: optional i32 getValue;
}

struct HwmonTestData {
  1: list<string> expectedFeatures;
}

struct WatchdogTestData {
  1: i32 numWatchdogs;
}

struct LedTestData {
  1: bool createsLeds;
}

// Runtime configuration structure - final configuration used in BspTests
// built by combining PlatformManager config with BspTestConfig data
struct RuntimeConfig {
  1: string platform;
  2: list<PciDevice> devices;
  3: map<string, I2CAdapter> i2cAdapters;
  /* pmName : adapter */
  4: platform_manager_config.BspKmodsFile kmods;
  5: map<string, DeviceTestData> testData;
  6: map<string, map<ExpectedErrorType, string>> expectedErrors;
}

struct PciDeviceInfo {
  1: string vendorId;
  2: string deviceId;
  3: string subSystemVendorId;
  4: string subSystemDeviceId;
}

struct PciDevice {
  1: string pmName;
  2: PciDeviceInfo pciInfo;
  7: list<fbiob_device_config.AuxData> auxDevices;
}

// Either a PCI device i2cAdapter,
// an i2cDevice MUX, or CPU adapter
struct I2CAdapter {
  1: string pmName;
  2: string busName;
  3: bool isCpuAdapter;
  4: optional PciAdapterInfo pciAdapterInfo;
  5: optional I2CMuxAdapterInfo muxAdapterInfo;
  6: list<I2CDevice> i2cDevices;
}

struct PciAdapterInfo {
  1: PciDeviceInfo pciInfo;
  2: fbiob_device_config.AuxData auxData;
}

struct I2CMuxAdapterInfo {
  @thrift.Box
  1: optional I2CAdapter parentAdapter;
  2: i32 parentAdapterChannel;
  3: i32 numOutgoingChannels;
  4: string deviceName;
  5: string address;
}

struct I2CDevice {
  1: string pmName;
  2: i32 channel;
  3: string deviceName;
  4: string address;
  5: bool isGpioChip;
}
