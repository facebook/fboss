// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager

enum DetectionMechanism {
  GPIO = 1,
  SYSFS = 2,
}

struct SysfsFileHandle {
  1: string sysfsPath;
  2: string desiredValue;
}

enum GpioValue {
  GPIO_LOW = 0,
  GPIO_HIGH = 1,
}

// A GPIO line can be specified by either <gpioChip + offset> pair, or
// global-unique <lineName> assigned in ACPI or Device Tree.
//   - "gpioChip" must be the symlink name under /run/devmap/gpiochips/.
//   - "lineName" is optional. If supplied, <gpioChip + offset> pair
//     will be updated at runtime, based on the result of gpiofind.
//   - GPIO lines are active high by default, unless "activeLow" is set
//     to true explicitly.
struct GpioLine {
  1: string gpioChip;
  2: i32 offset;
  3: optional string lineName;
  4: bool activeLow = false;
}

struct GpioLineHandle {
  1: GpioLine gLine;
  2: GpioValue desiredValue;
}

struct PresenceDetection {
  1: DetectionMechanism detectionMechanism;
  2: GpioLineHandle gpioHandle;
  3: optional SysfsFileHandle sysfsHandle;
}
