// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager
namespace py3 fboss.platform.platform_manager

// `devicePath`: DevicePath of the device which will create the sysfs file
// indicating the presence.
//
// `presenceFileName`: The name of the file which will contain the presence
// information.
//
// `desiredValue`: The value which will indicate that the device is present.
struct SysfsFileHandle {
  1: string devicePath;
  2: string presenceFileName;
  3: i16 desiredValue;
}

// `devicePath`: DevicePath of the gpiochip which holds the presence
// information.
//
// `lineIndex`: The gpio line number which will indicate the presence of the
// device.
//
// `desiredValue`: The value which will indicate that the device is present.
struct GpioLineHandle {
  1: string devicePath;
  2: i32 lineIndex;
  3: i16 desiredValue;
}

// The presence detection can be based on GPIO registers or sysfs paths.  Only
// one of them should be populated
struct PresenceDetection {
  1: optional GpioLineHandle gpioLineHandle;
  2: optional SysfsFileHandle sysfsFileHandle;
}
