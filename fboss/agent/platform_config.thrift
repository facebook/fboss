#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.platform_config
namespace py3 neteng.fboss.platform_config
namespace py.asyncio neteng.fboss.asyncio.platform_config
namespace cpp2 facebook.fboss.cfg
namespace go neteng.fboss.platform_config
namespace php fboss_platform_config

include "fboss/agent/hw/bcm/bcm_config.thrift"
include "fboss/agent/hw/sai/config/asic_config.thrift"
include "fboss/lib/phy/phy.thrift"
include "fboss/agent/switch_config.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"

enum PlatformAttributes {
  CONNECTION_HANDLE = 1,
  MAC = 2,
}

union ChipConfig {
  1: bcm_config.BcmConfig bcm;
  2: asic_config.AsicConfig asic;
}

struct PlatformConfig {
  1: ChipConfig chip;
  3: optional map<PlatformAttributes, string> platformSettings;
}

struct PlatformPortEntry {
  1: PlatformPortMapping mapping;
  2: map<switch_config.PortProfileID, PlatformPortConfig> supportedProfiles;
}

struct PlatformPortMapping {
  1: i32 id;
  2: string name;
  3: i32 controllingPort;
  4: list<phy.PinConnection> pins;
  5: switch_config.PortType portType = switch_config.PortType.INTERFACE_PORT;
  6: optional i32 attachedCoreId;
  7: optional i32 attachedCorePortIndex;
}

struct PlatformPortConfig {
  1: optional list<i32> subsumedPorts;
  2: phy.PortPinConfig pins;
}

// Currently we have 'PlatformPortConfig' in PlatformPortEntry to define the
// subsumedPorts and PortPinConfig(mainly tx settings) based on the specific
// port and specific PortProfileID. This PlatformPortConfig is usually unique.
// However, we also come across the following cases:
// 1) In som platform, this PlatformPortConfig is the same for multiple ports
// even regardless of the PortProfileID. For example, in Wedge400, for all the
// downlink ports, we'll use the same TX settings no matter of what speed we use
// 2) In case like Hardware engineer wants to play with the tx_settings for
// specific port and override the default settings from PlatformPortEntry
// To support the above cases, we decided to introduce this
// `PlatformPortConfigOverride`, which is kinda like a lookup table. So if
// list<PlatformPortConfigOverride> portConfigOverrides is defined in
// `PlatformMapping` for the specific platform, we will first check whether
// a port matches to any onverride.factor. If so, we'll directly to use
// override.pins to get the tx settings; otherwise, we will fall back to use
// the default tx_settings from `PlatformPortEntry`

// This struct will define an override factor.
// When deciding whether it's a match:
// 1) All the fields here are using "&&", which means if any field is set,
// we need to match every field.
// 2) While the elements in one field are using "||"
// For example:
// ports:[1, 2], profiles:[PROFILE_100G_4_NRZ_RS528_COPPER], means:
// port 1 with PROFILE_100G_4_NRZ_RS528_COPPER
// **OR**
// port 2 with PROFILE_100G_4_NRZ_RS528_COPPER
// is a match
struct PlatformPortConfigOverrideFactor {
  1: optional list<i32> ports;
  2: optional list<switch_config.PortProfileID> profiles;
  3: optional list<double> cableLengths;
  5: optional transceiver.TransceiverManagementInterface transceiverManagementInterface;
  6: optional list<phy.DataPlanePhyChip> chips;
  7: optional transceiver.MediaInterfaceCode mediaInterfaceCode;
}

struct PlatformPortConfigOverride {
  1: PlatformPortConfigOverrideFactor factor;
  2: optional phy.PortPinConfig pins;
  3: optional phy.PortProfileConfig portProfileConfig;
}

/*
  We introduce PlatformPortConfigFactor as the set of
  all possible factors that can contribute to profile config selection and
  introduce a list of PlatformPortProfileConfigEntry
  As an example, we can have a mixed pim platform with the following
  PlatformPortProfileConfigEntrys:
  [
    {
      factor: {
        profileID: P1,
        pimIDs: [1, 2, 3]
      },
      profile: conf1
    },
    {
      factor: {
        profileID: P1,
        pimIDs: [4, 5, 6]
      },
      profile: conf2
    }
  ]
  In the above example, a query (profileID=P1, pimID=2) will equate to conf1,
  while a query (profileID=P1, pimID=6) will equate to conf2, thereby supporting
  different profile configs, udner the same profileID P1, depending on pimID
*/
struct PlatformPortConfigFactor {
  1: switch_config.PortProfileID profileID;
  2: optional set<i32> pimIDs;
}

struct PlatformPortProfileConfigEntry {
  1: PlatformPortConfigFactor factor;
  2: phy.PortProfileConfig profile;
}

// TODO: Will deprecate the optional fields in PlatformConfig and start using
// this new struct in agent code
struct PlatformMapping {
  1: map<i32, PlatformPortEntry> ports;
  3: list<phy.DataPlanePhyChip> chips;
  4: optional map<PlatformAttributes, string> platformSettings;
  5: optional list<PlatformPortConfigOverride> portConfigOverrides;
  7: list<PlatformPortProfileConfigEntry> platformSupportedProfiles;
}
