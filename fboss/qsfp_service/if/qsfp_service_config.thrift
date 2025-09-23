#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace cpp2 facebook.fboss.cfg
namespace go neteng.fboss.qsfp_service_config
namespace py neteng.fboss.qsfp_service_config
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.qsfp_service_config

include "fboss/qsfp_service/if/transceiver_validation.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"
include "fboss/agent/switch_config.thrift"

struct QsfpSdkVersion {
  // The version associated with the desired Sdk
  2: string version;
}

enum TransceiverPartNumber {
  UNKNOWN = 1,
}

// The matching logic will treat the absent of an optional field as match all.
struct TransceiverConfigOverrideFactor {
  1: optional TransceiverPartNumber transceiverPartNumber;
  2: optional transceiver.SMFMediaInterfaceCode applicationCode;
  3: optional transceiver.ActiveCuHostInterfaceCode hostApplicationCode;
}

struct Sff8636Overrides {
  1: optional i16 rxPreemphasis;
  2: optional i16 rxAmplitude;
  3: optional i16 txEqualization;
}

struct CmisOverrides {
  1: optional transceiver.RxEqualizerSettings rxEqualizerSettings;
}

union TransceiverOverrides {
  1: Sff8636Overrides sff;
  2: CmisOverrides cmis;
}

struct TransceiverConfigOverride {
  1: TransceiverConfigOverrideFactor factor;
  2: TransceiverOverrides config;
}

struct CabledTestPair {
  1: string aPortName;
  2: string zPortName;
  3: switch_config.PortProfileID profileID;
}

struct QsfpTestConfig {
  1: list<CabledTestPair> cabledPortPairs;
  2: TransceiverFirmware firmwareForUpgradeTest;
}

enum FirmwareType {
  APPLICATION = 1,
  DSP = 2,
}

struct FirmwareVersion {
  1: FirmwareType fwType;
  2: string version;
}

struct Firmware {
  1: list<FirmwareVersion> versions;
}

struct TransceiverFirmware {
  // Transceiver Part Number to Firmware version(s) map
  1: map<string, Firmware> versionsMap;
}

struct TransceiverI2cLogging {
  1: bool writeLog;
  2: bool readLog;
  3: bool disableOnFail;
  4: i32 bufferSlots;
}

// Center optical channel frequnecy can be specified in either of the following ways
// 1. Frequency in MHz
// 2. Channel number
union CenterFrequencyConfig {
  1: i32 frequencyMhz;
  2: i32 channelNumber;
}

// Frequency config can be specified in either of the following ways
// 1. Frequency grid + Frequency(MhZ), internally software will calculate channel number based on frequency and grid
// And program the module with channel number and grid.
// 2. Frequency grid + Channel number, directly program the module with channel number and grid.
struct FrequencyConfig {
  1: transceiver.FrequencyGrid frequencyGrid;
  2: CenterFrequencyConfig centerFrequencyConfig;
}

struct OpticalChannelConfig {
  // Frequency config
  1: FrequencyConfig frequencyConfig;
  // Transmitter power in 0.01 dBm
  2: i32 txPower0P01Dbm;
  // Application Select code published by the META ZR spec
  3: i32 appSelCode;
}

struct QsfpServiceConfig {
  // This is used to override the default command line arguments we
  // pass to qsfp service.
  1: map<string, string> defaultCommandLineArgs = {};

  // This has a list of overrides of settings on the optic together with the
  // factor that specify the condition to apply.
  2: list<TransceiverConfigOverride> transceiverConfigOverrides = [];

  // Sdk versions for QSFP service
  3: optional QsfpSdkVersion sdk_version;

  4: optional QsfpTestConfig qsfpTestConfig;

  5: optional TransceiverFirmware transceiverFirmwareVersions;

  6: optional TransceiverI2cLogging transceiverI2cLogging;

  7: optional list<
    transceiver_validation.VendorConfig
  > transceiverValidationConfig;

  // Port name to optical channel config map
  8: optional map<string, OpticalChannelConfig> tunableOpticsConfig;
}
