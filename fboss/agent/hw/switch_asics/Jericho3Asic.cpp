// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {

bool Jericho3Asic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::OBJECT_KEY_CACHE:
    case HwAsic::Feature::PKTIO:
    case HwAsic::Feature::HOSTTABLE:
    case HwAsic::Feature::OBM_COUNTERS:
    case HwAsic::Feature::MIRROR_PACKET_TRUNCATION:
    case HwAsic::Feature::SFLOW_SAMPLING:
    case HwAsic::Feature::TELEMETRY_AND_MONITORING:
    case HwAsic::Feature::WIDE_ECMP:
    case HwAsic::Feature::ALPM_ROUTE_PROJECTION:
    case HwAsic::Feature::MAC_AGING:
    case HwAsic::Feature::SAI_PORT_SPEED_CHANGE: // CS00011784917
    case HwAsic::Feature::EGRESS_MIRRORING:
    case HwAsic::Feature::EGRESS_SFLOW:
    case HwAsic::Feature::DEFAULT_VLAN:
    case HwAsic::Feature::CPU_PORT:
    case HwAsic::Feature::VRF:
    case HwAsic::Feature::SAI_HASH_FIELDS_CLEAR_BEFORE_SET:
    case HwAsic::Feature::SAI_WEIGHTED_NEXTHOPGROUP_MEMBER:
    case HwAsic::Feature::PORT_TX_DISABLE:
    case HwAsic::Feature::SAI_PORT_ERR_STATUS:
    case HwAsic::Feature::ROUTE_PROGRAMMING:
    case HwAsic::Feature::FABRIC_PORTS:
    case HwAsic::Feature::LINK_TRAINING:
    case HwAsic::Feature::FEC:
    case HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE:
    case HwAsic::Feature::PMD_RX_LOCK_STATUS:
    case HwAsic::Feature::PMD_RX_SIGNAL_DETECT:
    case HwAsic::Feature::MEDIA_TYPE:
    case HwAsic::Feature::PORT_FABRIC_ISOLATE:
    case HwAsic::Feature::CPU_TX_VIA_RECYCLE_PORT:
    case HwAsic::Feature::SWITCH_DROP_STATS:
    case HwAsic::Feature::PACKET_INTEGRITY_DROP_STATS:
    case HwAsic::Feature::SAI_CONFIGURE_SIX_TAP:
    case HwAsic::Feature::DRAM_ENQUEUE_DEQUEUE_STATS:
    case HwAsic::Feature::RESOURCE_USAGE_STATS:
    case HwAsic::Feature::LINK_INACTIVE_BASED_ISOLATE:
    case HwAsic::Feature::SAI_FEC_COUNTERS:
    case HwAsic::Feature::SAI_FEC_CORRECTED_BITS:
    case HwAsic::Feature::BLACKHOLE_ROUTE_DROP_COUNTER:
    case HwAsic::Feature::ECN:
    case HwAsic::Feature::SAI_ECN_WRED:
    case HwAsic::Feature::QUEUE_ECN_COUNTER:
    case HwAsic::Feature::MANAGEMENT_PORT:
    case HwAsic::Feature::ANY_ACL_DROP_COUNTER:
    case HwAsic::Feature::EGRESS_FORWARDING_DROP_COUNTER:
    case HwAsic::Feature::ANY_TRAP_DROP_COUNTER:
    case HwAsic::Feature::ACL_COUNTER_LABEL:
    case HwAsic::Feature::ACL_COPY_TO_CPU:
    case HwAsic::Feature::SWITCH_ATTR_INGRESS_ACL:
    case HwAsic::Feature::ACL_TABLE_GROUP:
    case HwAsic::Feature::ERSPANv4:
    case HwAsic::Feature::ERSPANv6:
    case HwAsic::Feature::RCI_WATERMARK_COUNTER:
    case HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER:
    case HwAsic::Feature::SAI_PRBS:
    case HwAsic::Feature::PORT_SERDES_ZERO_PREEMPHASIS:
    case HwAsic::Feature::LINK_ACTIVE_INACTIVE_NOTIFY:
    case HwAsic::Feature::WARMBOOT:
    case HwAsic::Feature::PQP_ERROR_EGRESS_DROP_COUNTER:
    case HwAsic::Feature::FABRIC_LINK_DOWN_CELL_DROP_COUNTER:
    case HwAsic::Feature::SAI_FEC_CODEWORDS_STATS:
    case HwAsic::Feature::CRC_ERROR_DETECT:
    case HwAsic::Feature::ACL_METADATA_QUALIFER:
    case HwAsic::Feature::L3_MTU_ERROR_TRAP:
    case HwAsic::Feature::EVENTOR_PORT_FOR_SFLOW:
    case HwAsic::Feature::SFLOWv6:
    case HwAsic::Feature::ZERO_SDK_WRITE_WARMBOOT:
    case HwAsic::Feature::CPU_VOQ_BUFFER_PROFILE:
    case HwAsic::Feature::SWITCH_REACHABILITY_CHANGE_NOTIFY:
    case HwAsic::Feature::CABLE_PROPOGATION_DELAY:
    case HwAsic::Feature::DRAM_BLOCK_TIME:
    case HwAsic::Feature::VOQ_LATENCY_WATERMARK_BIN:
    case HwAsic::Feature::ACL_ENTRY_ETHER_TYPE:
    case HwAsic::Feature::ACL_BYTE_COUNTER:
    case HwAsic::Feature::EGRESS_CORE_BUFFER_WATERMARK:
    case HwAsic::Feature::DELETED_CREDITS_STAT:
    case HwAsic::Feature::INGRESS_PRIORITY_GROUP_DROPPED_PACKETS:
    case HwAsic::Feature::ROUTE_METADATA:
    case HwAsic::Feature::NO_RX_REASON_TRAP:
    case HwAsic::Feature::EGRESS_GVOQ_WATERMARK_BYTES:
    case HwAsic::Feature::INGRESS_PRIORITY_GROUP_SHARED_WATERMARK:
      return true;
    // Features not expected to work on SIM
    case HwAsic::Feature::SHARED_INGRESS_EGRESS_BUFFER_POOL:
    case HwAsic::Feature::BUFFER_POOL:
    case HwAsic::Feature::PFC:
    case HwAsic::Feature::PFC_XON_TO_XOFF_COUNTER:
    case HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET:
    case HwAsic::Feature::VOQ:
    case HwAsic::Feature::FABRIC_TX_QUEUES:
    case HwAsic::Feature::VOQ_DELETE_COUNTER:
    case HwAsic::Feature::L3_QOS:
    case HwAsic::Feature::TC_TO_QUEUE_QOS_MAP_ON_SYSTEM_PORT:
    case HwAsic::Feature::CREDIT_WATCHDOG:
    case HwAsic::Feature::SAI_PORT_SERDES_PROGRAMMING:
      return getAsicMode() != AsicMode::ASIC_MODE_SIM;
    // SIM specific features.
    case HwAsic::Feature::SAI_PORT_ETHER_STATS:
    case HwAsic::Feature::SLOW_STAT_UPDATE:
      // supported only on the SIM
      return getAsicMode() == AsicMode::ASIC_MODE_SIM;
    case HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE:
    case HwAsic::Feature::UDF_HASH_FIELD_QUERY:
    case HwAsic::Feature::IN_PAUSE_INCREMENTS_DISCARDS:
    case HwAsic::Feature::SAI_LAG_HASH:
    case HwAsic::Feature::QOS_MAP_GLOBAL:
    case HwAsic::Feature::QCM:
    case HwAsic::Feature::SMAC_EQUALS_DMAC_CHECK_ENABLED:
    case HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::PORT_INTERFACE_TYPE:
    case HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER:
    case HwAsic::Feature::HSDK:
    case HwAsic::Feature::L3_EGRESS_MODE_AUTO_ENABLED:
    case HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER:
    case HwAsic::Feature::PENDING_L2_ENTRY:
    case HwAsic::Feature::EGRESS_QUEUE_FLEX_COUNTER:
    case HwAsic::Feature::INGRESS_L3_INTERFACE:
    case HwAsic::Feature::NON_UNICAST_HASH:
    case HwAsic::Feature::DETAILED_L2_UPDATE:
    case HwAsic::Feature::COUNTER_REFRESH_INTERVAL:
    case HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD:
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT: // CS00012066057
    case HwAsic::Feature::MACSEC:
    case HwAsic::Feature::SAI_MPLS_QOS:
    case HwAsic::Feature::EMPTY_ACL_MATCHER:
    case HwAsic::Feature::ROUTE_FLEX_COUNTERS:
    case HwAsic::Feature::FEC_DIAG_COUNTERS:
    case HwAsic::Feature::SAI_ACL_TABLE_UPDATE:
    case HwAsic::Feature::SAI_MPLS_TTL_1_TRAP:
    case HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER:
    case HwAsic::Feature::EXACT_MATCH:
    case HwAsic::Feature::RX_FREQUENCY_PPM:
    case HwAsic::Feature::ECMP_MEMBER_WIDTH_INTROSPECTION:
    case HwAsic::Feature::SAI_FIRMWARE_PATH:
    // On Jericho3 ASIC we don't create any vlans but rather
    // associate RIFs directly with ports. Hence no bridge port
    // is created (or supported for now).
    case HwAsic::Feature::BRIDGE_PORT_8021Q:
    case HwAsic::Feature::FABRIC_PORT_MTU:
    case HwAsic::Feature::EXTENDED_FEC:
    case HwAsic::Feature::SAI_RX_REASON_COUNTER:
    case HwAsic::Feature::SAI_MPLS_INSEGMENT:
    case HwAsic::Feature::XPHY_PORT_STATE_TOGGLE:
    case HwAsic::Feature::SAI_PORT_GET_PMD_LANES:
    case HwAsic::Feature::SAI_PORT_VCO_CHANGE:
    case HwAsic::Feature::FLOWLET:
    case HwAsic::Feature::P4_WARMBOOT:
    case HwAsic::Feature::FEC_AM_LOCK_STATUS:
    case HwAsic::Feature::PCS_RX_LINK_STATUS:
    case HwAsic::Feature::SAI_CONFIGURE_SEVEN_TAP:
    case HwAsic::Feature::SAI_SAMPLEPACKET_TRAP:
    case HwAsic::Feature::SAI_UDF_HASH:
    case HwAsic::Feature::PTP_TC:
    case HwAsic::Feature::PTP_TC_PCS:
    case HwAsic::Feature::INGRESS_PRIORITY_GROUP_HEADROOM_WATERMARK:
    case HwAsic::Feature::RX_LANE_SQUELCH_ENABLE:
    case HwAsic::Feature::SEPARATE_BYTE_AND_PACKET_ACL_COUNTER:
    case HwAsic::Feature::FLOWLET_PORT_ATTRIBUTES:
    case HwAsic::Feature::SAI_EAPOL_TRAP:
    case HwAsic::Feature::SAI_USER_DEFINED_TRAP:
    case HwAsic::Feature::PORT_EYE_VALUES:
    case HwAsic::Feature::ECMP_DLB_OFFSET:
    case HwAsic::Feature::SPAN:
    case HwAsic::Feature::SFLOWv4:
    case HwAsic::Feature::MPLS:
    case HwAsic::Feature::MPLS_ECMP:
    case HwAsic::Feature::RX_SNR:
    case HwAsic::Feature::FEC_CORRECTED_BITS:
    case HwAsic::Feature::ROUTE_COUNTERS:
    // J3-AI natively supports hashing. So hash configuration is not supported.
    case HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION:
    case HwAsic::Feature::ECMP_HASH_V4:
    case HwAsic::Feature::ECMP_HASH_V6:
    case HwAsic::Feature::TRAFFIC_HASHING:
    case HwAsic::Feature::PORT_WRED_COUNTER:
    case HwAsic::Feature::DTL_WATERMARK_COUNTER:
    case HwAsic::Feature::MULTIPLE_ACL_TABLES:
    case HwAsic::Feature::SAI_ECMP_HASH_ALGORITHM:
    case HwAsic::Feature::SCHEDULER_PPS:
    case HwAsic::Feature::DATA_CELL_FILTER:
    case HwAsic::Feature::MULTIPLE_EGRESS_BUFFER_POOL:
    case HwAsic::Feature::ENABLE_DELAY_DROP_CONGESTION_THRESHOLD:
      return false;
  }
  return false;
}

std::set<cfg::StreamType> Jericho3Asic::getQueueStreamTypes(
    cfg::PortType portType) const {
  switch (portType) {
    case cfg::PortType::CPU_PORT:
      return {cfg::StreamType::UNICAST};
    case cfg::PortType::INTERFACE_PORT:
    case cfg::PortType::MANAGEMENT_PORT:
    case cfg::PortType::RECYCLE_PORT:
    case cfg::PortType::EVENTOR_PORT:
      return {cfg::StreamType::UNICAST};
    case cfg::PortType::FABRIC_PORT:
      return {cfg::StreamType::FABRIC_TX};
  }
  throw FbossError(
      "Jericho3 ASIC does not support:",
      apache::thrift::util::enumNameSafe(portType));
}
int Jericho3Asic::getDefaultNumPortQueues(
    cfg::StreamType streamType,
    cfg::PortType portType) const {
  if (getAsicMode() == AsicMode::ASIC_MODE_SIM) {
    // SIM will continue to have no queues though.
    return 0;
  }
  switch (streamType) {
    case cfg::StreamType::UNICAST:
      switch (portType) {
        case cfg::PortType::CPU_PORT:
        case cfg::PortType::RECYCLE_PORT:
        case cfg::PortType::INTERFACE_PORT:
        case cfg::PortType::MANAGEMENT_PORT:
        case cfg::PortType::EVENTOR_PORT:
          return 8;
        case cfg::PortType::FABRIC_PORT:
          break;
      }
      break;
    case cfg::StreamType::MULTICAST:
      break;
    case cfg::StreamType::FABRIC_TX:
      if (portType != cfg::PortType::FABRIC_PORT) {
        break;
      }
      return 1;
    case cfg::StreamType::ALL:
      break;
  }
  throw FbossError(
      "Unexpected, stream: ",
      apache::thrift::util::enumNameSafe(streamType),
      " portType: ",
      apache::thrift::util::enumNameSafe(portType),
      " combination");
}

uint64_t Jericho3Asic::getDefaultReservedBytes(
    cfg::StreamType streamType,
    cfg::PortType portType) const {
  switch (portType) {
    case cfg::PortType::CPU_PORT:
      return 1778;
    case cfg::PortType::RECYCLE_PORT:
      return 4096;
    case cfg::PortType::INTERFACE_PORT:
    case cfg::PortType::MANAGEMENT_PORT:
    case cfg::PortType::FABRIC_PORT:
    case cfg::PortType::EVENTOR_PORT:
      return 0;
  }
  throw FbossError(
      "Unexpected, stream: ",
      apache::thrift::util::enumNameSafe(streamType),
      " portType: ",
      apache::thrift::util::enumNameSafe(portType),
      " combination");
}
cfg::Range64 Jericho3Asic::getReservedEncapIndexRange() const {
  // Reserved range worked out with vendor. These ids
  // are reserved in SAI-SDK implementation for use
  // by NOS
  return makeRange(0x200000, 0x300000);
}

HwAsic::RecyclePortInfo Jericho3Asic::getRecyclePortInfo() const {
  return {
      .coreId = 2,
      .corePortIndex = 2,
      .speedMbps = 100000 // 100G
  };
}

const std::map<cfg::PortType, cfg::PortLoopbackMode>&
Jericho3Asic::desiredLoopbackModes() const {
  static const std::map<cfg::PortType, cfg::PortLoopbackMode> kLoopbackMode = {
      {cfg::PortType::INTERFACE_PORT, cfg::PortLoopbackMode::PHY},
      {cfg::PortType::MANAGEMENT_PORT, cfg::PortLoopbackMode::PHY},
      {cfg::PortType::FABRIC_PORT, cfg::PortLoopbackMode::MAC},
      {cfg::PortType::RECYCLE_PORT, cfg::PortLoopbackMode::NONE},
      {cfg::PortType::EVENTOR_PORT, cfg::PortLoopbackMode::NONE}};
  return kLoopbackMode;
}

HwAsic::AsicMode Jericho3Asic::getAsicMode() const {
  static const char* kSimPath = std::getenv("BCM_SIM_PATH");
  if (kSimPath) {
    return AsicMode::ASIC_MODE_SIM;
  }
  return AsicMode::ASIC_MODE_HW;
}

std::optional<uint32_t> Jericho3Asic::computePortGroupSkew(
    const std::map<PortID, uint32_t>& portId2cableLen) const {
  std::map<int, uint32_t> portGroup2MaxCableLen;
  auto updatePortGroupMax = [&portGroup2MaxCableLen](
                                int groupId, uint32_t cableLen) {
    auto pgItr = portGroup2MaxCableLen.find(groupId);
    auto currentMax = pgItr != portGroup2MaxCableLen.end() ? pgItr->second : 0;
    portGroup2MaxCableLen[groupId] = std::max(currentMax, cableLen);
  };
  static auto const kPortGroups = getPortGroups();
  for (auto [portId, cableLen] : portId2cableLen) {
    auto portIdInt = static_cast<int>(portId);
    for (auto g = 0; g < kPortGroups.size(); ++g) {
      auto [portGroupStart, portGroupEnd] = kPortGroups.at(g);
      if (portIdInt >= portGroupStart && portIdInt <= portGroupEnd) {
        updatePortGroupMax(g, cableLen);
        continue;
      }
    }
  }
  std::set<uint32_t> portGroupMaxLensSorted;
  std::for_each(
      portGroup2MaxCableLen.begin(),
      portGroup2MaxCableLen.end(),
      [&portGroupMaxLensSorted](auto groupAndLen) {
        portGroupMaxLensSorted.insert(groupAndLen.second);
      });
  if (portGroupMaxLensSorted.empty()) {
    return std::nullopt;
  }
  return *portGroupMaxLensSorted.rbegin() - *portGroupMaxLensSorted.begin();
}

std::vector<std::pair<int, int>> Jericho3Asic::getPortGroups() const {
  // J3 has fabric ports organized in 4 groups of
  // 40 ports each starting at port id 1024
  constexpr auto kPortGroupStart = 1024;
  constexpr auto kPortGroupSize = 40;
  constexpr auto kNumPortGroups = 4;
  std::vector<std::pair<int, int>> portGroups;
  for (auto g = 0; g < kNumPortGroups; ++g) {
    auto portGroupStart = kPortGroupStart + g * kPortGroupSize;
    auto portGroupEnd = portGroupStart + kPortGroupSize - 1;
    portGroups.push_back(std::make_pair(portGroupStart, portGroupEnd));
  }
  return portGroups;
}
} // namespace facebook::fboss
