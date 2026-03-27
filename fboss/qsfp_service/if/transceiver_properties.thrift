// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

namespace cpp2 facebook.fboss
namespace go neteng.fboss.transceiver_properties
namespace py neteng.fboss.transceiver_properties
namespace py3 neteng.fboss

include "fboss/agent/switch_config.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"

package "facebook.com/fboss/qsfp_service/transceiver_properties"

// A contiguous range of lanes (host or media side).
struct LaneRange {
  1: i16 start;
  2: i16 count;
}

// A single port's contribution to a speed combination.
struct PortSpeedConfig {
  // Port speed for this segment
  1: switch_config.PortSpeed speed;
  // Host-side lane range
  2: LaneRange hostLanes;
  // Media-side lane range (may differ from host for gearbox optics)
  3: LaneRange mediaLanes;
  // Raw EEPROM media interface byte value for all media lanes in this port
  4: transceiver.MediaInterfaceUnion mediaLaneCode;
  // The MediaInterfaceCode that this mediaLaneCode maps to
  5: transceiver.MediaInterfaceCode mediaInterfaceCode = transceiver.MediaInterfaceCode.UNKNOWN;
}

// A valid speed combination for multi-port optics.
// Each entry is a list of per-port configs; startHostLane determines placement.
struct SpeedCombination {
  // Human-readable name (e.g., "2x400G-FR4", "8x100G-DR1")
  1: string combinationName;
  // Per-port configs (startHostLane determines lane placement, order irrelevant)
  2: list<PortSpeedConfig> ports;
}

// The first application advertisement from EEPROM, used to identify the module media interface.
struct FirstApplicationAdvertisement {
  // Media interface code (from SFF-8024 Table 4-7, e.g., DR4_800G)
  1: transceiver.MediaInterfaceUnion mediaInterfaceCode;
  // Host-side start lanes this application supports (e.g., [0,4] for dual-port)
  2: list<i16> hostStartLanes;
  // Host interface code byte (e.g., 0x25 = GAUI8_800G)
  // Note the type ActiveCuHostInterfaceCode is misnamed, it is actually the HostInterfaceCode byte
  3: transceiver.ActiveCuHostInterfaceCode hostInterfaceCode;
}

// Diagnostics capabilities that differ from defaults (all supported).
// When the struct is absent, all diagnostics are supported.
// Only set fields for capabilities that are NOT supported.
struct DiagnosticCapabilitiesExceptions {
  1: bool doesNotSupportVdm = false;
  2: bool doesNotSupportRxOutputControl = false;
}

// Properties of a single SMF transceiver (MediaInterfaceCode).
struct SmfTransceiverProperties {
  // The first application advertisement that identifies this transceiver
  1: FirstApplicationAdvertisement firstApplicationAdvertisement;

  // SMF fiber length in meters (e.g., 500 for DR4, 3000 for FR4).
  // Optional: when absent, smfLength is not used for derivation matching.
  2: optional i32 smfLength;

  // Lane configuration
  3: i16 numHostLanes;
  4: i16 numMediaLanes;

  // Human-readable name (e.g., "DR4_2x800G")
  5: string displayName;

  // Valid speed combinations
  6: list<SpeedCombination> supportedSpeedCombinations;

  // Diagnostics capabilities exceptions (absent means all supported)
  7: optional DiagnosticCapabilitiesExceptions diagnosticCapabilitiesExceptions;

  // Valid speed-change transitions
  8: list<list<string>> speedChangeTransitions = [];
}

// Top-level config
struct TransceiverPropertiesConfig {
  // SMF transceivers: keyed by MediaInterfaceCode integer value
  1: map<i32, SmfTransceiverProperties> smfTransceivers;
  // Future: activeCuTransceivers, passiveCuTransceivers
}
