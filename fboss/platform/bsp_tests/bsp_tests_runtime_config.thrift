// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.bsp_tests
include "thrift/annotation/thrift.thrift"

include "fboss/platform/platform_manager/platform_manager_config.thrift"
include "fboss/platform/bsp_tests/fbiob_device_config.thrift"
include "fboss/platform/bsp_tests/bsp_tests_config.thrift"

@thrift.AllowLegacyMissingUris
package;

// Runtime configuration structure - final configuration used in BspTests
// built by combining PlatformManager config with BspTestConfig data
struct RuntimeConfig {
  1: string platform;
  2: list<PciDevice> devices;
  3: map<string, I2CAdapter> i2cAdapters;
  /* pmName : adapter */
  4: platform_manager_config.BspKmodsFile kmods;
  5: map<string, bsp_tests_config.DeviceTestData> testData;
  6: map<
    string,
    map<bsp_tests_config.ExpectedErrorType, string>
  > expectedErrors;
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
