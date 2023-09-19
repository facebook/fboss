// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager

include "fboss/platform/platform_manager/platform_manager_presence.thrift"

// +-+-+-+ +-+-+-+ +-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+-+-+
// |I|2|C| |B|u|s| |N|a|m|i|n|g| |C|o|n|v|e|n|t|i|o|n|
// +-+-+-+ +-+-+-+ +-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+-+-+
//
// I2C bus names for a PmUnit are assigned from the PmUnit perspective.  From
// the PmUnit's perspective the origin of the bus can be external (coming
// directly from the slot), or can be a mux within a PmUnit.
//
// If the I2C Adapter name is known, then that should be used as bus name.  If
// not, use the below logic.
//
// If the source of the bus is the slot where the PmUnit is plugged in, then
// the bus is named INCOMING@<incoming_index>.  In the below example, PmUnit A
// has two incoming buses falling into this category.  Similarly PmUnit B has
// three incoming buses named as INCOMING@<incoming_index>
//
// If the source of the bus is a mux within a PmUnit, then the bus is assigned
// the name <MUX_NAME>@<channel_number>.  The MUX_NAME, which is represented as
// `I2cDeviceConfig::pmUnitScopedName` should be unique for each mux within the
// PmUnit. For example, in PmUnit A, the INCOMING@1 bus gets muxed into muxA@0,
// muxA@1 and muxA@2.  So, the outgoing buses out of the PmUnit Slot from
// PmUnit A are muxA@0, INCOMING@0 and muxA@2.
//
// If the source of the bus is a FPGA within a PmUnit, then the bus is assigned
// the name <FRU_SCOPED_NAME> of the I2C Adapter within the FPGA.
//
// Note, the three incoming buses of PmUnit B are assigned the names
// INCOMING@0, INCOMING@1 and INCOMING@2.  These names are independent of how
// the buses originated from PmUnit A.
//                                            PmUnit
//            ┌────────────────────┐         Boundary        ┌────────────────────┐
//            │      PmUnit A ┌────┤            │            │     PmUnit B       │
//            │  ┌────┬─────┐ │    │                         │   ┌────┬─────┐     │
//  INCOMING@0│  │ 12 │     │ │    │muxA@0      │  INCOMING@0│   │ 12 │     │┌────┤
// ───────────┼┬▶├────┘     │┌┼────┼─────────────────────────┼┬─▶├────┘     ││    │
//            ││ │ sensor1  │││    │            │            ││  │ sensor1  ││Slot│
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

// `I2cDeviceConfig` defines a i2c device within any PmUnit.
//
// `busName`: Refer to Bus Naming Convention above.
//
// `address`: I2c address used by the device in hex notation
//
// `kernelDeviceName`: The device name used by kernel to identify the device
//
// `pmUnitScopeName`: The name assigned to the device in the config, unique
// within the scope of PmUnit.
//
// `numOutgoingChannels`: Number of outgoing channels (applies only for mux)
//
// `isEeprom`: Indicates whether this device is an EEPROM. If not specified, it
// defaults to false.
//
// `isChassisEeprom`: Indicates whether this device is the Chassis EEPROM. If
// not specified, it defaults to false.
//
// For example, the three i2c devices in the below Sample PmUnit will be modeled
// as follows
//
// sensor1 = I2cDeviceConfig( busName="INCOMING@0", address="0x12",
// kernelDeviceName="lm75", pmUnitScopeName="sensor1")
//
// sensor2 = I2cDeviceConfig( busName="mux1@0", address="0x13",
// kernelDeviceName="lm75", pmUnitScopeName="sensor2")
//
// mux1 = I2cDeviceConfig( busName="INCOMING@1", address="0x54",
// kernelDeviceName="pca9x48", pmUnitScopeName="mux1", numOutgoingChannels=3)
//                    ┌──────────────────────────────────────────┐
//                    │               Sample PmUnit              │
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
  2: string address;
  3: string kernelDeviceName;
  4: string pmUnitScopedName;
  5: optional i32 numOutgoingChannels;
  6: bool isEeprom;
  7: bool isChassisEeprom;
}

// The IDPROM which contains information about the PmUnit or Chassis
//
// `busName`: This bus should be directly from the CPU, or an incoming bus into
// the PmUnit (i.e., there should not be any mux or fpga in between).  In the
// case of former, the I2C Adapter name should be used, and in the case of
// latter, the INCOMING@ notation should be used. Note, this bus can originate
// from a mux/fpga in an upstream PmUnit.
//
// `address`: I2C address of the IDPROM in hex notation
//
// `kernelDeviceName`: The device name used by kernel to identify the device
struct IdpromConfig {
  1: string busName;
  2: string address;
  3: string kernelDeviceName;
}

// Defines I2C Adapter in FPGAs.
//
// `i2cAdapterName`: It is the name used in the ioctl system call to create
// the i2c adapter. It should one of the compatible strings specified in the
// kernel driver.
//
// `offset`: It is the memory offset of the I2C Adapter in the FPGA.
struct I2cAdapterConfig {
  1: string i2cAdapterName;
  3: i32 offset;
}

// Defines the SPI Master in FPGAs.
//
// `spiMasterName`: It is the name used in the ioctl system call to create
// the spi master. It should one of the compatible strings specified in the
// kernel driver.
//
// `offset`: It is the memory offset of the spi master in the FPGA.
//
// `numberOfCsPins`: Number of CS (chip-select) pins.
struct SpiMasterConfig {
  1: string spiMasterName;
  3: i32 offset;
  4: i32 numberOfCsPins;
}

// Defines the GPIO Chip in FPGAs.
//
// `gpioChipName`: It is the name used in the ioctl system call to create
// the gpio chip. It should one of the compatible strings specified in the
// kernel driver.
//
// `offset`: It is the memory offset of the gpio chip in the FPGA.
struct GpioChipConfig {
  1: string gpioChipName;
  3: i32 offset;
}

// `vendorId`: PCIe Vendor ID, and it must be a 4-digit heximal value, such as
// “1d9b”
//
// `deviceId`: PCIe Device ID, and it must be a 4-digit heximal value, such as
// “0011”
//
// `i2cAdapterConfigs`: Lists the I2C Adapters in the PMUnit. The key is the
// PMUnit scoped name of the I2C Adapter.
//
// `spiMasterConfigs`: Lists the SPI Masters in the PMUnit. The key is the
// PMUnit scoped name of the SPI Master.
//
// `gpioChipConfigs`: Lists the GPIO Chips in the PMUnit. The key is the
// PMUnit scoped name of the GPIO Chip.
//
// TODO: Add MDIO support
struct PciDevice {
  1: string vendorId;
  2: string deviceId;
  3: map<string, I2cAdapterConfig> i2cAdapterConfigs;
  4: map<string, SpiMasterConfig> spiMasterConfigs;
  5: map<string, GpioChipConfig> gpioChipConfigs;
}

// These are the PmUnit slot types. Examples: "PIM_SLOT", "PSU_SLOT" and
// "FAN_SLOT"
typedef string SlotType

// The below struct holds the global properties for each SlotType within any
// platform.  This means all slots of the same SlotType within a platform
// should have the same number of outgoing I2C buses, and same IdpromConfig. At
// least one of idpromConfig or pmUnitName should be present.
//
// If both are present, the exploration will use pmUnitName to proceed with
// exploration.
//
// Also, if both are present, the pmUnitName in idprom contents should match
// pmUnitName defined here.  The exploration will warn if there is mismatch of
// pmUnitName.
struct SlotTypeConfig {
  1: i32 numOutgoingI2cBuses;
  2: optional IdpromConfig idpromConfig;
  3: optional string pmUnitName;
}

// SlotConfig holds information specific to each slot.
//
// `slotType`: Type of the slot. Examples: "PIM_SLOT", "PSU_SLOT"  and
// "FAN_SLOT".
//
// `presenceDetection`: Logic to determine whether a PmUnit has been plugged in
// this slot.
//
// TODO: Enhance device presence logic based on SimpleIoDevice definition in
// fbdevd.thrift
//
// `outgoingI2cBusNames`: is the list of the buses from the PmUnit perspective
// which are going out in the slot.  Refer to Bus Naming Convention above.
struct SlotConfig {
  1: SlotType slotType;
  2: platform_manager_presence.PresenceDetection presenceDetection;
  3: list<string> outgoingI2cBusNames;
}

// `PmUnitConfig` defines the configuration of PmUnit.
//
// `pluggedInSlotType`: The SlotType where the PmUnit is plugged in.
//
// `i2cDeviceConfigs`: List of I2cDeviceConfigs on the PmUnit
//
// `outgoingSlotConfigs`: Details about the slots present on the PmUnit. Slot
// Name is the key.
//
struct PmUnitConfig {
  1: SlotType pluggedInSlotType;
  2: list<I2cDeviceConfig> i2cDeviceConfigs;
  3: map<string, SlotConfig> outgoingSlotConfigs;
  4: list<PciDevice> pciDevices;
}

// Defines the whole Platform. The top level struct.
struct PlatformConfig {
  // Name of the platform.  Should match the name set in dmedicode
  1: string platformName;

  // This is the PmUnit from which the exploration will begin. The IDPROM of
  // this PmUnit should be directly connected to the CPU SMBus.
  2: string rootPmUnitName;

  // Map from SlotType name to the global properties of the SlotType.
  11: map<SlotType, SlotTypeConfig> slotTypeConfigs;

  // List of PmUnits which the platform can support. Key is the PmUnit name.
  12: map<string, PmUnitConfig> pmUnitConfigs;

  // List of the i2c buses created from the CPU / System Control Module (SCM)
  // We are assuming the i2c Adapter name (content of
  // /sys/bus/i2c/devices/i2c-N/name) is unique for buses directly coming of
  // CPU. We have to revisit this logic if this assumption changes.
  13: list<string> i2cAdaptersFromCpu;

  // Global mapping from the i2c device paths to an application friendly sysfs
  // path.
  14: map<string, string> i2cPathToHumanFriendlyName;

  // Name and version of the rpm containing the BSP kmods for this platform
  21: string bspKmodsRpmName;
  22: string bspKmodsRpmVersion;

  // Name and version of the rpm containing the udev rules for this platform
  23: string udevRpmName;
  24: string udevRpmVersion;
}
