// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.fbdevd
namespace go neteng.fboss.platform.fbdevd
namespace py neteng.fboss.platform.fbdevd
namespace py3 neteng.fboss.platform.fbdevd
namespace py.asyncio neteng.fboss.platform.asyncio.fbdevd

include "fboss/platform/fbdevd/if/gpio.thrift"
include "fboss/platform/fbdevd/if/i2c.thrift"

// This file contains data types for describing the hardware components
// and device topology of FBOSS switches.

// "SysfsFileHandle" is designed for reading/writing sysfs files.
//   - "sysfsPath" must be an absolute path, or a symlink to an absolute
//     path.
//   - If "isWrite" is true, "desiredValue" will be written to the sysfs
//     entry.
//   - If "isWrite" is false (default), it will trigger file read, and
//     callers may compare the return data with "desiredValue".
//
// Please refer to below URLs for Linux sysfs interface:
//   - https://www.kernel.org/doc/Documentation/filesystems/sysfs.txt
//   - https://www.kernel.org/doc/html/latest/admin-guide/sysfs-rules.html
struct SysfsFileHandle {
  1: string sysfsPath;
  2: string desiredValue;
  3: bool isWrite = false;
}

// "SimpleIO" in this context refers to a simple (one-line) instruction,
// such as toggling a GPIO line, reading/writing a sysfs file, etc.
// Such instructions are widely used in FBOSS environment to configure
// hardware and/or determine the states of hardware components.
// Currently, only 2 operations are support in "SimpleIO":
//   - SIO_GPIO : setting/getting a GPIO line.
//   - SIO_SYSFS: reading/writing a string from/to SYSFS file.
enum SimpleIoDeviceType {
  SIO_GPIO = 1,
  SIO_SYSFS = 2,
}

union SimpleIoDevice {
  1: gpio.GpioLineHandle gpioHandle;
  2: SysfsFileHandle sysfsHandle;
}

struct SimpleIoHandle {
  1: SimpleIoDeviceType ioType;
  2: SimpleIoDevice device;

  // If "delayMS" is set to a positive value, the corresponding delay (
  // in milliseconds) will be introduced after the SimpleIO. Otherwise,
  // the field is ignored.
  3: i32 delayMs = 0;
}

enum ConditionType {
  COND_GPIO_MATCH = 1,
  COND_SYSFS_MATCH = 2,
}

// "ConditionMatch" is used to define conditional statement, to test if
// a certain GPIO line or sysfs file reports desired value.
struct ConditionMatch {
  1: ConditionType condType;
  2: SimpleIoDevice device;
}

// Trigger a list of SimpleIO when a certain condition is met.
struct ConditionalSimpleIo {
  1: list<SimpleIoHandle> actions;
  2: optional ConditionMatch isRequired;
}

// "FruModule" is used to describe a FRU (Field Replaceable Unit).
struct FruModule {
  1: string fruName;

  // Check if a FRU is plugged/removed by testing if the given GPIO line
  // or sysfs file reports the desired value.
  2: ConditionMatch fruState;

  // Instructions to initialize the FRU when it's plugged.
  // It's normal to keep the lists empty.
  3: ConditionalSimpleIo initOps;

  // Instructions to clean up resources when the FRU is unplugged.
  // It's normal to keep the lists empty.
  4: list<SimpleIoHandle> cleanupOps;

  // I2C client devices on the FRU. The devices need to be created when
  // the FRU is plugged, and will be removed when the FRU is unplugged.
  5: list<i2c.I2cClient> i2cClients;

  // Nested FRUs if any.
  6: list<FruModule> childFruList;
}

// Structure to describe a specific FBOSS switch.
struct fbossPlatformDesc {
  1: string platformName;

  // List of kernel modules that need to be loaded. It's no-op if the
  // modules were already loaded.
  2: list<string> kmods;

  // List of instructions to bootstrap the platform, such as toggling
  // GPIOs, bringing devices out of reset, etc.
  // It's normal to have empty "bootstrap".
  3: ConditionalSimpleIo bootstrap;

  // I2C client devices on the chassis that need to be instantiated after
  // system boot up. It's no-op if some devices were already created, and
  // we don't remove these devices when "fbdevd" exits.
  4: list<i2c.I2cClient> i2cClients;

  // List of FRUs.
  5: list<FruModule> fruList;
}

// No service functions at present.
service FbdevManager {
}
