// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.bsp_tests.fbiob

/*
 * Corresponds to platform_manager/uapi/fbiob-ioctl.h
 */

struct AuxData {
  1: string name;
  2: AuxDeviceType type;
  3: AuxID id;
  4: optional string csrOffset;
  5: optional string iobufOffset;
  6: optional FanData fanData;
  7: optional I2cData i2cData;
  8: optional SpiData spiData;
  9: optional LedData ledData;
  10: optional XcvrData xcvrData;
}

struct AuxID {
  1: string deviceName;
  2: i32 id;
}

enum AuxDeviceType {
  FAN = 0,
  I2C = 1,
  SPI = 2,
  LED = 3,
  XCVR = 4,
  GPIO = 5,
}

struct I2cData {
  1: optional i32 busFreqHz;
  2: optional i32 numChannels;
}

struct SpiDevInfo {
  1: string modalias;
  2: i32 chipSelect;
  3: i32 maxSpeedHz;
}

struct SpiData {
  1: i32 numSpidevs;
  2: list<SpiDevInfo> spiDevices;
}

struct LedData {
  1: i32 portNumber;
  2: i32 ledId;
}

struct XcvrData {
  1: i32 portNumber;
}

struct FanData {
  1: i32 numFans;
}
