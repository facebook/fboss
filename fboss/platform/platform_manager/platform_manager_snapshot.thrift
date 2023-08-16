// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager

// The structs below are mostly a mirror of the structs defined in
// platform_manager_config.thrift.  The structs below will reflect the
// discovered state of the platform on an alive switch.  Some high level
// aspects in which the contents will differ between platform_mananger_snapshot
// and platform_manager_config are as follows.
//
// - The i2c bus names in `PlatformSnapshot` will reflect the kernel assigned
// names, e.g., i2c-9.  The i2c bus names on `PlatformConfig` will follow the
// convention described in platform_manager_config.thrift
// - The Slot struct in `PlatformSnapshot` will point to the FRU plugged in the
// slot.  `PlatformConfig.SlotType` will have details to detect whether a FRU
// has been plugged in that slot.

typedef string SlotType

struct I2cDevice {
  1: string busName;
  2: i32 addr;
  3: string kernelDeviceName;
  4: string fruScopedName;
  5: optional i32 numOutgoingChannels;
}

struct Slot {
  1: SlotType slotType;
  2: list<string> outgoingI2cBusNames;
  3: optional Fru pluggedInFru;
}

struct Fru {
  1: string name;
  2: SlotType pluggedInSlotType;
  3: list<I2cDevice> i2cDevices;
  4: map<string, Slot> outgoingSlots;
}

struct PlatformSnapshot {
  1: string platformName;
  2: Fru mainBoardFru;
  3: Slot mainBoardSlot;
  4: list<string> i2cAdaptersFromCpu;
  5: map<string, string> i2cPathToHumanFriendlyName;
}
