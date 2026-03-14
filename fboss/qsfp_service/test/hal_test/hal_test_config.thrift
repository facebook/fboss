// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

include "fboss/qsfp_service/if/qsfp_service_config.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"

package "facebook.com/fboss/qsfp_service/test/hal_test"

namespace cpp2 facebook.fboss

struct HalTestStartupConfig {
  1: optional qsfp_service_config.Firmware firmware;
}

struct HalTestTransceiverEntry {
  1: i32 id;
  2: string name;
  // Optional path overrides (used only when no platform is specified)
  3: optional string i2cDevicePath;
  4: optional string presentPath;
  5: optional string resetPath;
  6: optional HalTestStartupConfig startupConfig;
}

struct HalTestMediaInterfaceConfig {
  1: list<TcvrOperationalMode> supportedModes = [];
  2: list<list<TcvrOperationalMode>> speedChangeTransitions = [];
}

struct HalTestConfig {
  1: list<HalTestTransceiverEntry> transceivers;
  // Per-media-interface test configuration. If empty, defaults are used.
  2: map<
    transceiver.MediaInterfaceCode,
    HalTestMediaInterfaceConfig
  > mediaInterfaceConfigs = {};
}

enum TcvrOperationalMode {
  MODE_2x400G_FR4 = 0,
  MODE_8x100G_FR1 = 1,
  MODE_400G_FR4_200G_FR4 = 2,
  MODE_200G_FR4_400G_FR4 = 3,
  MODE_2x200G_FR4 = 4,
  MODE_2x800G_DR4 = 5,
  MODE_8x200G_DR1 = 6,
  MODE_4x400G_DR2 = 7,
  MODE_1x800G_FR8 = 8,
  MODE_8x100G_DR1 = 9,
}

const map<
  transceiver.MediaInterfaceCode,
  HalTestMediaInterfaceConfig
> DEFAULT_MEDIA_INTERFACE_CONFIGS = {
  transceiver.MediaInterfaceCode.FR4_2x400G: {
    "supportedModes": [
      TcvrOperationalMode.MODE_2x400G_FR4,
      TcvrOperationalMode.MODE_400G_FR4_200G_FR4,
      TcvrOperationalMode.MODE_200G_FR4_400G_FR4,
      TcvrOperationalMode.MODE_2x200G_FR4,
      TcvrOperationalMode.MODE_8x100G_FR1,
    ],
    "speedChangeTransitions": [
      [
        TcvrOperationalMode.MODE_2x400G_FR4,
        TcvrOperationalMode.MODE_2x200G_FR4,
      ],
      [
        TcvrOperationalMode.MODE_2x200G_FR4,
        TcvrOperationalMode.MODE_200G_FR4_400G_FR4,
      ],
      [
        TcvrOperationalMode.MODE_2x400G_FR4,
        TcvrOperationalMode.MODE_400G_FR4_200G_FR4,
      ],
    ],
  },
  transceiver.MediaInterfaceCode.DR4_2x800G: {
    "supportedModes": [
      TcvrOperationalMode.MODE_2x800G_DR4,
      TcvrOperationalMode.MODE_4x400G_DR2,
      TcvrOperationalMode.MODE_8x200G_DR1,
      TcvrOperationalMode.MODE_8x100G_DR1,
    ],
    "speedChangeTransitions": [],
  },
};
