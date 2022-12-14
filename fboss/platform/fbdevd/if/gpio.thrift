// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.fbdevd.gpio

enum GpioValue {
  GPIO_LOW = 0,
  GPIO_HIGH = 1,
}

enum GpioDirection {
  GPIO_DIR_OUT = 0,
  GPIO_DIR_IN = 1,
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

// Structure to deal with GPIO Line access (set/get GPIO).
//  - If "isWrite" is True, the GPIO line's direction is set to OUT,
//    and line level is updated to "desiredValue".
//  - If "isWrite" is False, the GPIO line's direction is set to IN. The
//    line is read, and callers may compare "desiredValue" with the
//    current line level and take actions.
struct GpioLineHandle {
  1: GpioLine gLine;
  2: GpioValue desiredValue;
  3: bool isWrite = false;
}
