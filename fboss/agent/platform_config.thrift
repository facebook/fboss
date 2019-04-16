#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.platform_config
namespace py.asyncio neteng.fboss.asyncio.platform_config
namespace cpp2 facebook.fboss.cfg

include "fboss/agent/hw/bcm/bcm_config.thrift"
include "fboss/lib/phy/external_phy.thrift"
include "fboss/agent/switch_config.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"

union ChipConfig {
  1: bcm_config.BcmConfig bcm,
}

struct FrontPanelResources {
  1: i32 transceiverId,
  2: list<i16> channels,
}

struct AsicTxSettings {
  // TX equalizer settings for the asic internal phy
  1: optional external_phy.TxSettings tx,

  // which type of fec should be enabled
  2: optional external_phy.FecMode fec,
}

struct TxConfig {
  1: map<transceiver.TransmitterTechnology, AsicTxSettings> settings,

  // TODO: Support overriding tx/fec settings by length/gauge?
}

struct PlatformPortSettings {
  // Settings for configuring an external phy at the given speed, if the platform has external phys
  1: optional external_phy.ExternalPhyPortSettings xphy,

  // Corresponding front panel resources for the port at this speed
  2: optional FrontPanelResources frontPanelResources,

  // Ports that are subsumed and cannot be enabled at this speed
  3: list<i32> subsumedPorts = [],

  // Settings for asic side tx
  4: optional TxConfig tx,
}

struct PlatformPort {
  1: i32 id,
  2: string name,
  3: map<switch_config.PortSpeed, PlatformPortSettings> supportedSpeeds,
}

struct PlatformConfig {
  1: ChipConfig chip,
  2: map<i32, PlatformPort> ports,
}
