// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager

include "fboss/platform/platform_manager/platform_manager_presence.thrift"

// +-+-+-+ +-+-+-+ +-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+-+-+
// |I|2|C| |B|u|s| |N|a|m|i|n|g| |C|o|n|v|e|n|t|i|o|n|
// +-+-+-+ +-+-+-+ +-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+-+-+
//
// I2C bus names for a FRU are assigned from the FRU perspective.  From the
// FRU's perspective the origin of the bus can be external (coming directly
// from the slot), or can be a mux within a FRU.
//
// If the source of the bus is the slot where the FRU is plugged in, then the
// bus is named INCOMING@<incoming_index>.  In the below example, FRU A has two
// incoming buses falling into this category.  Similarly FRU B has three
// incoming buses named as INCOMING@<incoming_index>
//
// If the source of the bus is a mux within a FRU, then the bus is assigned the
// name <MUX_NAME>@<channel_number>.  The MUX_NAME, which is represented as
// `I2cDeviceConfig::fruScopedName` should be unique for each mux within the
// FRU. For example, in FRU A, the INCOMING@1 bus gets muxed
// into muxA@0, muxA@1 and muxA@2.  So, the outgoing buses out of the FRU Slot
// from FRU A are muxA@0, INCOMING@0 and muxA@2.
//
// Note, the three incoming buses of FRU B are assigned the names INCOMING@0,
// INCOMING@1 and INCOMING@2.  These names are independent of how the buses
// originated from FRU A.
//                                             FRU
//            ┌────────────────────┐         Boundary        ┌────────────────────┐
//            │       FRU A   ┌────┤            │            │       FRU B        │
//            │  ┌────┬─────┐ │    │                         │   ┌────┬─────┐     │
//  INCOMING@0│  │ 12 │     │ │    │muxA@0      │  INCOMING@0│   │ 12 │     │┌────┤
// ───────────┼┬▶├────┘     │┌┼────┼─────────────────────────┼┬─▶├────┘     ││FRU │
//            ││ │ sensor1  │││FRU │            │            ││  │ sensor1  ││Slot│
//            ││ └──────────┘││Slot│                         ││  └──────────┘│    │
//            ││             ││    │            │            ││              │    │INCOMING@0
//            ││             ││    │INCOMING@0     INCOMING@1│└──────────────┼────┼─────────▶
//            │└─────────────┼┼────┼────────────┼────────────┼──▶            │    │
//            │              ││    │                         │               │    │muxB@0
//            │ ┌────────┐   ││    │            │            │ ┌────────┐  ┌─┼────┼─────────▶
//  INCOMING@1│ │  muxA  ├───┘│    │                         │ │  muxB  ├──┘ │    │
// ───────────┼▶├────┐   ├─▶  │    │muxA@2      │  INCOMING@2│ ├────┐   ├─▶  │    │muxB@2
//            │ │ 54 │   ├────┼────┼─────────────────────────┼▶│ 54 │   ├────┼────┼─────────▶
//            │ └────┴───┘    └────┤            │            │ └────┴───┘    │    │
//            │                    │                         │               └────┤
//            └────────────────────┘            │            └────────────────────┘
//
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// `I2cDeviceConfig` defines a i2c device within any FRU.
//
// `busName`: Refer to Bus Naming Convention above.
//
// `addr`: I2c address used by the i2c device
//
// `kernelDeviceName`: The device name used by kernel to identify the device
//
// `fruScopeName`: The name assigned to the device in the config, unique within
// the scope of fru.
//
// `numOutgoingChannels`: Number of outgoing channels (applies only for mux)
//
// For example, the three i2c devices in the below sample_fru will be modeled
// as follows
//
// sensor1 = I2cDeviceConfig( busName="INCOMING@0", addr=12, kernelDeviceName="lm75",
// fruScopeName="sensor1")
//
// sensor2 = I2cDeviceConfig( busName="mux1@0", addr=13, kernelDeviceName="lm75",
// fruScopeName="sensor2")
//
// mux1 = I2cDeviceConfig( busName="INCOMING@1", addr=54, kernelDeviceName="pca9x48",
// fruScopeName="mux1", numOutgoingChannels=3)
//                    ┌──────────────────────────────────────────┐
//                    │                sample_fru                │
//    INCOMING@0      │                       ┌────┬─────┐       │
// ───────────────────┼──────────────────────▶│ 12 │     │       │
//                    │                       ├────┘     │       │
//                    │                       │ sensor1  │       │
//                    │                       └──────────┘       │
//                    │                                          │
//                    │     ┌─────────┐  mux1@0   ┌────┬─────┐   │
//    INCOMING@1      │     │   mux1  ├──────────▶│ 13 │     │   │
// ───────────────────┼────▶├────┐    ├───▶       ├────┘     │   │
//                    │     │ 54 │    ├───▶       │ sensor2  │   │
//                    │     └────┴────┘           └──────────┘   │
//                    └──────────────────────────────────────────┘
struct I2cDeviceConfig {
  1: string busName;
  2: i32 addr;
  3: string kernelDeviceName;
  4: string fruScopedName;
  5: optional i32 numOutgoingChannels;
}

// The EEPROM in the FRU which contains information about the FRU
//
// `incomingBusIndex`: One of the incoming buses into the FRU should directly
// connect to the FRU. i.e., not a bus originating from a mux within the FRU.
// Note, this bus can originate from a mux in an upstream FRU.
//
// `kernelDeviceName`: The device name used by kernel to identify the device
struct FruEEPROMConfig {
  1: i32 incomingBusIndex;
  2: i32 address;
  3: string kernelDeviceName;
}

// These are the FRU Slot types. Examples: "PIM", "PSU", "CHASSIS" and "FAN".
typedef string SlotType

// These are the FRU Types. Examples: "PIM-8DD", "PIM-16Q".
typedef string FruType

// The below struct holds the global properties for each SlotType within any
// platform.  This means all slots of the same SlotType within a platform
// should have the same number of outgoing I2C buses, and same FruEEPROMConfig
struct SlotTypeConfig {
  1: i32 numOutgoingI2cBuses;
  2: FruEEPROMConfig fruEeprom;
}

// SlotConfig holds information specific to each slot.
//
// `slotType`: Type of the slot.
//
// `presenceDetection`: Logic to determine whether a FRU has been plugged in
// this slot.
//
// TODO: Enhance device presence logic based on SimpleIoDevice definition
// in fbdevd.thrift
//
// `outgoingI2cBusNames`: is the list of the buses from the FRU perspective
// which are going out in the slot.  Refer to Bus Naming Convention above.
struct SlotConfig {
  1: SlotType slotType;
  2: platform_manager_presence.PresenceDetection presenceDetection;
  3: list<string> outgoingI2cBusNames;
}

// `FruTypeConfig` defines the configuration of FRU.
//
// `pluggedInSlotType`: The SlotType where the FRU is plugged in.
//
// `i2cDeviceConfigs`: List of I2cDeviceConfigs on the FRU
//
// `outgoingSlotConfigs`: Details about the slots present on the FRU. Slot Name
// is the key.
//
struct FruTypeConfig {
  1: SlotType pluggedInSlotType;
  2: list<I2cDeviceConfig> i2cDeviceConfigs;
  3: map<string, SlotConfig> outgoingSlotConfigs;
}

// Defines the whole Platform. The top level struct.
struct PlatformConfig {
  // Name of the platform.  Should match the name set in dmedicode
  1: string platformName;

  // Each platform should have a Main Board FruTypeConfig defined.
  // This refers to the Switch Main Board (SMB)
  2: FruTypeConfig mainBoardFruTypeConfig;

  // chassisSlotConfig describes the virtual Chassis slot where the Chassis is
  // plugged in.
  3: SlotConfig mainBoardSlotConfig;

  // Map from SlotType name to the global properties of the SlotType.
  4: map<SlotType, SlotTypeConfig> slotTypeConfigs;

  // List of FRUs which the platform can support. Key is the FRU name.
  5: map<FruType, FruTypeConfig> fruTypeConfigs;

  // List of the i2c busses created from the CPU / System Control Module (SCM)
  6: list<string> i2cBussesFromCPU;

  // Global mapping from the i2c device paths to an application friendly sysfs
  // path.
  7: map<string, string> i2cPathToHumanFriendlyName;
}
