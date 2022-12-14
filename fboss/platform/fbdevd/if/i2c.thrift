// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.fbdevd.i2c
namespace go neteng.fboss.platform.fbdevd.i2c
namespace py neteng.fboss.platform.fbdevd.i2c
namespace py3 neteng.fboss.platform.fbdevd.i2c
namespace py.asyncio neteng.fboss.platform.asyncio.i2c

// "I2cClient" describes a I2C client (slave) device, such as I2C
// sensors, EEPROM, I/O Expander, etc.
//   - "bus" must be the symlink under /run/devmap/i2c-busses/.
//   - "deviceName" is the string for driver matching, such as tmp75.
struct I2cClient {
  1: string bus;
  2: i16 address;
  3: string deviceName;
}
