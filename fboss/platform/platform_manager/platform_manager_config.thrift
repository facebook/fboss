// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager
namespace py3 fboss.platform.platform_manager

include "fboss/platform/platform_manager/platform_manager_presence.thrift"

//            +-+-+-+ +-+-+-+ +-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+-+-+
//            |I|2|C| |B|u|s| |N|a|m|i|n|g| |C|o|n|v|e|n|t|i|o|n|
//            +-+-+-+ +-+-+-+ +-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+-+-+
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
// PmUnit. In the example, in PmUnit A, the INCOMING@1 bus gets muxed into
// muxA@0, muxA@1 and muxA@2.  So, the outgoing buses out of the PmUnit Slot
// from PmUnit A are muxA@0, INCOMING@0 and muxA@2.
//
// If the source of the bus is a FPGA within a PmUnit, then the bus is assigned
// the name <PmUnit_SCOPED_NAME> of the I2C Adapter within the FPGA. In the
// example below, fpga1_I2C_2 is the adapter name of the I2C bus coming out of
// the fpga.
//
// Note, the three incoming buses of PmUnit B are assigned the names
// INCOMING@0, INCOMING@1 and INCOMING@2.  These names are independent of how
// the buses originated from PmUnit A.
//                                             PmUnit
//            ┌────────────────────┐           Boundary      ┌────────────────────┐
//            │      PmUnit A ┌────┤            │            │      PmUnit B      │
//            │  ┌────┬─────┐ │Slot│                         │   ┌────┬─────┐     │
//  INCOMING@0│  │ 12 │     │ │    │muxA@0      │  INCOMING@0│   │ 12 │     │┌────┤
// ───────────┼┬▶├────┘     │┌┼────┼─────────────────────────┼┬─▶├────┘     ││Slot│
//            ││ │ sensor1  │││    │            │            ││  │ sensor1  ││    │
//            ││ └──────────┘││    │                         ││  └──────────┘│    │
//            ││             ││    │            │            ││              │    │INCOMING@0
//            ││             ││    │INCOMING@0     INCOMING@1│└──────────────┼────┼─────────▶
//            │└─────────────┼┼────┼────────────┼────────────┼──▶            │    │
//            │              ││    │                         │               │    │muxB@0
//            │ ┌────────┐   ││    │            │            │ ┌────────┐  ┌─┼────┼─────────▶
//  INCOMING@1│ │  muxA  ├───┘│    │                         │ │  muxB  ├──┘ │    │
// ───────────┼▶├────┐   ├─▶  │    │muxA@2      │  INCOMING@2│ ├────┐   ├─▶  │    │muxB@2
//            │ │ 54 │   ├────┼────┼─────────────────────────┼▶│ 54 │   ├────┼────┼─────────▶
//            │ └────┴───┘    └────┤            │            │ └────┴───┘    │    │
//            │         ┌────────┐ │                         │               │    │fpga1_I2C_2
//            │         │  muxC  │ │            │            │           ┌───┼────┼─────────▶
//            │         ├────┐   │ │                         │           │   │    │
//            │         │ 12 │   │ │                         │           │   │    │
//            │         └─▲──┴───┘ │                         │           │   │    │
//            │           │        │                         │           │   └────┤
//            │     SMBus │        │                         │           │        │
//            │    Adapter│        │                         │           │        │
//            │           │        │                         │           │        │
//            │ ┌─────────┴──────┐ │                         │ ┌─────────┴──────┐ │
//            │ │      CPU       │ │                         │ │     fpga1      │ │
//            │ └────────────────┘ │                         │ └────────────────┘ │
//            └────────────────────┘                         └────────────────────┘

// ============================================================================

//                             +-+-+-+-+-+-+-+-+
//                             |S|l|o|t|P|a|t|h|
//                             +-+-+-+-+-+-+-+-+
//
// SlotPaths are constructs used to reference slots in the platform. The
// virtual root slot is a forward slash (/).  The SlotPaths are constructed in
// the sequence of the slot names in which they are plugged in.  The separator
// between slot names is a forward slash (/).  For example, in the below
// platform,
// - Root PmUnit is plugged in SlotPath /
// - First XYZ PmUnit is plugged in SlotPath /XYZ_SLOT@0
// - Second XYZ PmUnit is plugged in SlotPath /XYZ_SLOT@1
// - ABC1 PmUnit is plugged in SlotPath /ABC_SLOT@0
// - ABC2 PmUnit is plugged in SlotPath /ABC_SLOT@1
// - DEF PmUnit is plugged in SlotPath /ABC_SLOT@1/DEF_SLOT@0
//
// ┌─────────────────┐   ┌──────────────────────────┐   ┌─────────────────┐
// │   ABC1 PmUnit   │   │       Root PmUnit        │   │   XYZ PmUnit    │
// │   ┌───────┐     │   ├──────────┐    ┌──────────┤   │   ┌───────┐     │
// │   │ cpld1 │     ├ ─ ┤ABC_SLOT@0│    │XYZ_SLOT@0├ ─ ┤   │sensor1│     │
// │   └───────┘     │   ├──────────┘    └──────────┤   │   └───────┘     │
// └─────────────────┘   │                          │   └─────────────────┘
//                       │                          │
// ┌─────────────────┐   │                          │   ┌─────────────────┐
// │   ABC2 PmUnit   │   ├──────────┐    ┌──────────┤   │   XYZ PmUnit    │
// │┌─────┐   ┌─────┐├ ─ ┤ABC_SLOT@1│    │XYZ_SLOT@1├ ─ ┤   ┌───────┐     │
// ││cpld1│   │cpld2││   ├──────────┘    └──────────┤   │   │sensor2│     │
// │└─────┘   └─────┘│   │                          │   │   └───────┘     │
// │   ┌──────────┐  │   │     ┌───────────────┐    │   └─────────────────┘
// │   │DEF_SLOT@0│  │   │     │     fpga1     │    │
// └───┴────┬─────┴──┘   │     │  ┌──────────┐ │    │
//                       │     │  │gpiochip0 │ │    │
// ┌────────┴────────┐   │     └──┴──────────┴─┘    │
// │   DEF PmUnit    │   │                          │
// │   ┌───────┐     │   └──────────────────────────┘
// │   │sensor3│     │
// │   └───────┘     │
// └─────────────────┘
//
// ============================================================================

//                            +-+-+-+-+-+-+-+-+-+-+
//                            |D|e|v|i|c|e|P|a|t|h|
//                            +-+-+-+-+-+-+-+-+-+-+
//
// DevicePaths are constructs used to refer to devices in the platform. To
// represent a device in a PmUnit, the path should contain the SlotPath where
// the PmUnit is plugged in, followed by the device name.  The device itself is
// represented within square brackets (e.g., [DeviceName]). The device should
// be the leaf (last token), of the path. I2C buses are also considered as
// devices
//
// The devices in the above example are represented as follows
// - /[fpga1]
// - /[gpiochip0]
// - /XYZ_SLOT@0/[sensor1]
// - /XYZ_SLOT@0/[INCOMING@0]
// - /XYZ_SLOT@1/[sensor1]
// - /XYZ_SLOT@1/[INCOMING@0]
// - /ABC_SLOT@0/[cpld1]
// - /ABC_SLOT@0/[INCOMING@0]
// - /ABC_SLOT@1/[cpld1]
// - /ABC_SLOT@1/[INCOMING@0]
// - /ABC_SLOT@1/[cpld2]
// - /ABC_SLOT@1/DEF_SLOT@0/[sensor3]
// - /ABC_SLOT@1/DEF_SLOT@0/[INCOMING@0]

// ============================================================================

// Defines desired value of the given I2C register(s).
//
// `regOffset`: I2C device register offset.
//
// `ioBuf`: Data to be written to the device register.
struct I2cRegData {
  1: i32 regOffset;
  2: list<byte> ioBuf;
}

// `I2cDeviceConfig` defines a i2c device within any PmUnit.
//
// `busName`: Refer to Bus Naming Convention above.
//
// `address`: I2c address used by the device in hex notation
//
// `kernelDeviceName`: The device name used by kernel to identify the device
//
// `pmUnitScopedName`: The name assigned to the device in the config, unique
// within the scope of PmUnit.
//
// `isGpioChip`: Whether this I2C Device is a GpioChip
//
// `numOutgoingChannels`: Number of outgoing channels (applies only for mux)
//
// `hasBmcMac`: Whether this has BMC MAC address (applies only to EEPROM)
//
// `hasCpuMac`: Whether this has CPU MAC address (applies only to EEPROM)
//
// `hasSwitchAsicMac`: Whether this has Switch ASIC MAC addresses (applies
// only to EEPROM)
//
// `hasReservedMac`: Whether this has Reserved MAC addresses (applies only to
// EEPROM)
//
// `initRegSettings`: initial I2c register values before device creation.
//
// `isWatchdog`: Whether this I2C Device is a Watchdog device
//
// For example, the three i2c devices in the below Sample PmUnit will be modeled
// as follows
//
// sensor1 = I2cDeviceConfig( busName="INCOMING@0", address="0x12",
// kernelDeviceName="lm75", pmUnitScopedName="sensor1")
//
// sensor2 = I2cDeviceConfig( busName="mux1@0", address="0x13",
// kernelDeviceName="lm75", pmUnitScopedName="sensor2")
//
// mux1 = I2cDeviceConfig( busName="INCOMING@1", address="0x54",
// kernelDeviceName="pca9x48", pmUnitScopedName="mux1", numOutgoingChannels=3)
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
  5: bool isGpioChip;
  6: optional i32 numOutgoingChannels;
  7: bool hasBmcMac;
  8: bool hasCpuMac;
  9: bool hasSwitchAsicMac;
  10: bool hasReservedMac;
  11: optional list<I2cRegData> initRegSettings;
  12: bool isWatchdog;
}

// Configs for sensors which are embedded (eg within CPU).
//
// `pmUnitScopedName`: The name used to refer to this device. It should be
// be unique within the PmUnit.
//
// `sysfsPath`: This is the path assigned to the device by the kernel. It
// is the path where the `hwmon` directory is present.
//
struct EmbeddedSensorConfig {
  1: string pmUnitScopedName;
  2: string sysfsPath;
}

// The IDPROM which contains information about the PmUnit.  The
// PmUnitScopedName of the IDPROM device is always just "IDPROM".
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
//
// `offset`: The offset at which Meta V5 IDPROM format resides.
struct IdpromConfig {
  1: string busName;
  2: string address;
  3: string kernelDeviceName;
  4: i16 offset;
}

// Defines a generic IP block in the FPGA
//
// `pmUnitScopedName`: The name used to refer to this device. It should be
// be unique within the PmUnit.
//
// `deviceName`: It is the name used in the ioctl system call to create the
// corresponding device. It should one of the compatible strings specified in
// the kernel driver.
//
// `iobufOffset`: It is the iobuf register hex offset of the SPI Master in the
// FPGA.
//
// `csrOffset`: It is the csr register hex offset of the SPI Master in the FPGA.
struct FpgaIpBlockConfig {
  1: string pmUnitScopedName;
  2: string deviceName;
  3: string iobufOffset;
  4: string csrOffset;
}

// Defines the I2C Adapter config in FPGAs.
//
// `fpgaIpBlockConfig`: See FgpaIpBlockConfig above
//
// `numberOfAdapters`: Number of I2C Adapters created by this block.
struct I2cAdapterConfig {
  1: FpgaIpBlockConfig fpgaIpBlockConfig;
  2: i32 numberOfAdapters;
}

// Defines a Spi Device in FPGAs.
//
// `pmUnitScopedName`: The name used to refer to this device. It should be be
// unique among other SpiSlaves and in associated pmUnit.
// SpiDeviceConfig.pmUnitScopedName is the name of the SpiSlave device, whereas
// SpiMasterConfig.fpgaIpBlockConfig.pmUnitScopedName is the name of the
// SpiMaster device.
//
// `modalias`: Type of SpiSlave Device. spi_dev or any id in
// https://github.com/torvalds/linux/blob/master/drivers/spi/spidev.c#L702
//
// `chipSelect`: Value of chip select on the board.
//
// `maxSpeedHz`: Maximum clock rate to be used with this chip on the board.
struct SpiDeviceConfig {
  1: string pmUnitScopedName;
  2: string modalias;
  3: i32 chipSelect;
  4: i32 maxSpeedHz;
}

// Defines the SPI Master block in FPGAs.
//
// `fpgaIpBlockConfig`: See FgpaIpBlockConfig above
//
// `spiDeviceConfigs`: See SpiDeviceConfig above.
struct SpiMasterConfig {
  1: FpgaIpBlockConfig fpgaIpBlockConfig;
  2: list<SpiDeviceConfig> spiDeviceConfigs;
}

// Defines the Fan/PWM Controller block in FPGAs
//
// `fpgaIpBlockConfig`: See FgpaIpBlockConfig above
//
// `numFans`: Number of fans which is associated with this config.
struct FanPwmCtrlConfig {
  1: FpgaIpBlockConfig fpgaIpBlockConfig;
  2: i32 numFans;
}

// Defines the Transceiver Controller block in FPGAs.
//
// `fpgaIpBlockConfig`: See FgpaIpBlockConfig above
//
// `portNumber`: Port number which is associated with this config.
struct XcvrCtrlConfig {
  1: FpgaIpBlockConfig fpgaIpBlockConfig;
  2: i32 portNumber;
}

// Defines the LED Controller block in FPGAs.
//
// `fpgaIpBlockConfig`: See FgpaIpBlockConfig above
//
// `portNumber`: Port number which is associated with this config. Used
// for port LEDs
//
// `ledId`: Led ID for this config.
struct LedCtrlConfig {
  1: FpgaIpBlockConfig fpgaIpBlockConfig;
  2: i32 portNumber;
  3: i32 ledId;
}

// Defines PCI Devices in the PmUnits. A new PciDeviceConfig should be created
// for each unique combination of <vendorId, deviceId, subSystemVendorId,
// subSystemDeviceId>.
//
// In the case of one device (e.g. DOM FPGA) memory mapped to another PCI
// Device (e.g. IOB FPGA) in a different PmUnit, all of them might show up as a
// single PCI device in the system.  In this case, the PciDeviceConfig with the
// same <vendorId, deviceId, subSystemVendorId, subSystemDeviceId> should be
// present in both PmUnitConfigs.  For example, if a DOM FPGA in SMB PmUnit is
// memory mapped to an IOB FPGA in MCB PmUnit, there should be a
// PciDeviceConfig in MCB PmUnitConfig listing all controllers getting created
// as part of IOB FPGA, and there should be another PciDeviceConfig with the
// same identifiers in PMUnitConfig of SMB listing all the controllers getting
// created as part of the DOM FPGA.
//
// `pmUnitScopedName`: The name assigned to the device in the config, unique
// within the scope of PmUnit.
//
// `vendorId`: PCIe Vendor ID, and it must be a 4-digit hexadecimal value, such
// as “1d9b”
//
// `deviceId`: PCIe Device ID, and it must be a 4-digit hexadecimal value, such
// as “0011”
//
// `subSystemVendorId`: PCIe Sub System Vendor ID, and it must be a 4-digit
// hexadecimal value, such as “1d9b”
//
// `subSystemDeviceId`: PCIe Sub System Device ID, and it must be a 4-digit
// hexadecimal value, such as “0011”
//
// The remaining fields are configs per controller block in the FPGA
//
// TODO: Add MDIO support
struct PciDeviceConfig {
  1: string pmUnitScopedName;
  2: string vendorId;
  3: string deviceId;
  4: string subSystemVendorId;
  5: string subSystemDeviceId;
  6: list<I2cAdapterConfig> i2cAdapterConfigs;
  7: list<SpiMasterConfig> spiMasterConfigs;
  8: list<FpgaIpBlockConfig> gpioChipConfigs;
  9: list<FpgaIpBlockConfig> watchdogConfigs;
  10: list<FanPwmCtrlConfig> fanTachoPwmConfigs;
  11: list<LedCtrlConfig> ledCtrlConfigs;
  12: list<XcvrCtrlConfig> xcvrCtrlConfigs;
  13: list<FpgaIpBlockConfig> infoRomConfigs;
  14: list<FpgaIpBlockConfig> miscCtrlConfigs;
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
// this slot. Need not be described if there is no presence detection for this
// slot
//
// TODO: Enhance device presence logic based on SimpleIoDevice definition in
// fbdevd.thrift
//
// `outgoingI2cBusNames`: is the list of the buses from the PmUnit perspective
// which are going out in the slot.  Refer to Bus Naming Convention above.
struct SlotConfig {
  1: SlotType slotType;
  2: optional platform_manager_presence.PresenceDetection presenceDetection;
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
struct PmUnitConfig {
  1: SlotType pluggedInSlotType;
  2: list<I2cDeviceConfig> i2cDeviceConfigs;
  3: map<string, SlotConfig> outgoingSlotConfigs;
  4: list<PciDeviceConfig> pciDeviceConfigs;
  5: list<EmbeddedSensorConfig> embeddedSensorConfigs;
}

// `VersionedPmUnitConfig` defined a configuration of PmUnit in re-spinned
// platforms.
//
// `PmUnitConfig`: PmUnit configuration. Refer to PmUnitConfig definition above.
//
// `productSubVersion`: The platformVersion of the switch which this PmUnit
// belongs to. This refers to field Type 10 in Meta EEPROM V5
// TODO: Replace it with PmUnitVersion struct below.
struct VersionedPmUnitConfig {
  1: PmUnitConfig pmUnitConfig;
  3: i16 productSubVersion;
}

// `PmUnitInfo`: Details of a PmUnit.
//
// `name`: Name of the PmUnit.
//
// `version`: Version of the pmUnit.
struct PmUnitInfo {
  1: string name;
  2: optional PmUnitVersion version;
}

// `PmUnitVersion`: Version of a PmUnit.
//
// `productProductionState`: Minimum productProductionState (EEPROM V5 Type 8).
//
// `productVersion`: Minimum productVersion (EEPROM V5 Type 9).
//
// `productSubVersion`: Minimum productSubVersion (EEPROM V5 Type 10).
struct PmUnitVersion {
  1: i16 productProductionState;
  2: i16 productVersion;
  3: i16 productSubVersion;
}

// Defines thrift structure used for the Bsp Kmods file under /usr/local/{vendor}_bsp/...
// This file will be written during BSP development. PM will consume this file to unload
// specified kmods before exploration.
//
// `bspKmods`: Specify the list of names of bsp kmods on the installed rpm.
//
// `sharedKmods`: Specify the list of names of shared kmods on the installed rpm.
// These shared kmods will be unloaded after bspKmods.
struct BspKmodsFile {
  1: list<string> bspKmods;
  2: list<string> sharedKmods;
}

// Defines the whole Platform. The top level struct.
struct PlatformConfig {
  // Name of the platform.  Should match the name set in dmidecode
  1: string platformName;

  // This is the PmUnit from which the exploration will begin. The IDPROM of
  // this PmUnit should be directly connected to the CPU SMBus.
  2: string rootPmUnitName;

  // This is the SlotType of the rootPmUnit.
  3: string rootSlotType;

  // Map from SlotType name to the global properties of the SlotType.
  11: map<SlotType, SlotTypeConfig> slotTypeConfigs;

  // List of PmUnits which the platform can support. Key is the PmUnit name.
  12: map<string, PmUnitConfig> pmUnitConfigs;

  // List of the i2c buses created from the CPU.  We are assuming the i2c
  // Adapter name (content of /sys/bus/i2c/devices/i2c-N/name) is unique for
  // buses directly coming of CPU. We have to revisit this logic if this
  // assumption changes.
  13: list<string> i2cAdaptersFromCpu;

  // Global mapping from an application friendly path (symbolic link) to
  // DevicePath. DevicePath documentation can be found earlier in the file
  14: map<string, string> symbolicLinkToDevicePath;

  // Map from PmUnit name to a list of PmUnitConfigs which apply to specific
  // versions of the platform.  This typically applies to re-spins and
  // second-source boards/PmUnits.
  15: map<string, list<VersionedPmUnitConfig>> versionedPmUnitConfigs;

  // Name and version of the rpm containing the BSP kmods for this platform
  21: string bspKmodsRpmName;
  22: string bspKmodsRpmVersion;

  // Specify the list of names of bsp kmods to be reloaded on new rpm
  // installation.  On every invocation of the program, they will be unloaded
  // and reloaded
  23: list<string> bspKmodsToReload;

  // Specify the list of names of shared kmods to be reloaded on new rpm
  // installation.  These shared kmods must be loaded before all other bsp
  // kmods and unloaded after all other bsp kmods.
  24: list<string> sharedKmodsToReload;

  // Specify the list of non-bsp kmods which are used by the system. They are
  // only loaded and never unloaded.
  25: list<string> upstreamKmodsToLoad;
}
