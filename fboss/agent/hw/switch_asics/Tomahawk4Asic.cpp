// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"

DECLARE_int32(acl_gid);

namespace {
// On TH4, LOGICAL_TABLE_ID is 4 bit which will give 16 groups per pipe.
// From IFP point of view the device operate in 4 Pipes which will get
// 4*16 = 64 groups.
// However in older devices LOGICAL_TABLE_ID is 5 bit which will give you
// 128 groups.
// However SDK reserves Group 64, to update the group qset even when entries
// already installed in the group.
// So 63 is the largest group id we can get.
constexpr auto kDefaultACLGroupID = 63;
// On TH4 the range for EM group Id is 0-15.
// Hence 16 is the total number EM group Id can be created.
constexpr auto kDefaultTeFlowGroupID = 1;
} // namespace

namespace facebook::fboss {

bool Tomahawk4Asic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::SPAN:
    case HwAsic::Feature::ERSPANv4:
    case HwAsic::Feature::SFLOWv4:
    case HwAsic::Feature::MPLS:
    case HwAsic::Feature::MPLS_ECMP:
    case HwAsic::Feature::ERSPANv6:
    case HwAsic::Feature::SFLOWv6:
    case HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION:
    case HwAsic::Feature::ECN:
    case HwAsic::Feature::L3_QOS:
    case HwAsic::Feature::SCHEDULER_PPS:
    case HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::DEBUG_COUNTER:
    case HwAsic::Feature::RESOURCE_USAGE_STATS:
    case HwAsic::Feature::HSDK:
    case HwAsic::Feature::OBJECT_KEY_CACHE:
    case HwAsic::Feature::L3_EGRESS_MODE_AUTO_ENABLED:
    case HwAsic::Feature::PKTIO:
    case HwAsic::Feature::ACL_COPY_TO_CPU:
    case HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER:
    case HwAsic::Feature::OBM_COUNTERS:
    case HwAsic::Feature::BUFFER_POOL:
    case HwAsic::Feature::EGRESS_QUEUE_FLEX_COUNTER:
    case HwAsic::Feature::INGRESS_L3_INTERFACE:
    case HwAsic::Feature::DETAILED_L2_UPDATE:
    case HwAsic::Feature::TELEMETRY_AND_MONITORING:
    case HwAsic::Feature::ALPM_ROUTE_PROJECTION:
    case HwAsic::Feature::MAC_AGING:
    case HwAsic::Feature::SAI_PORT_SPEED_CHANGE: // CS00011784917
    case HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD:
    case HwAsic::Feature::EGRESS_MIRRORING:
    case HwAsic::Feature::EGRESS_SFLOW:
    case HwAsic::Feature::DEFAULT_VLAN:
    case HwAsic::Feature::L2_LEARNING:
    case HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER:
    case HwAsic::Feature::TRAFFIC_HASHING:
    case HwAsic::Feature::ACL_TABLE_GROUP:
    case HwAsic::Feature::CPU_PORT:
    case HwAsic::Feature::VRF:
    case HwAsic::Feature::SAI_HASH_FIELDS_CLEAR_BEFORE_SET:
    case HwAsic::Feature::ROUTE_COUNTERS:
    case HwAsic::Feature::ROUTE_FLEX_COUNTERS:
    case HwAsic::Feature::BRIDGE_PORT_8021Q:
    case HwAsic::Feature::FEC_DIAG_COUNTERS:
    case HwAsic::Feature::PTP_TC:
    case HwAsic::Feature::PTP_TC_PCS:
    case HwAsic::Feature::SAI_LAG_HASH:
    case HwAsic::Feature::PMD_RX_LOCK_STATUS:
    case HwAsic::Feature::PMD_RX_SIGNAL_DETECT:
    case HwAsic::Feature::SAI_FEC_COUNTERS:
    case HwAsic::Feature::SAI_WEIGHTED_NEXTHOPGROUP_MEMBER:
    case HwAsic::Feature::SAI_ECN_WRED:
    case HwAsic::Feature::MULTIPLE_ACL_TABLES:
    case HwAsic::Feature::SWITCH_ATTR_INGRESS_ACL:
    case HwAsic::Feature::EXACT_MATCH:
    case HwAsic::Feature::ROUTE_PROGRAMMING:
    case HwAsic::Feature::ECMP_HASH_V4:
    case HwAsic::Feature::ECMP_HASH_V6:
    case HwAsic::Feature::FEC_CORRECTED_BITS:
    case HwAsic::Feature::MEDIA_TYPE:
    case HwAsic::Feature::FEC:
    case HwAsic::Feature::ECMP_MEMBER_WIDTH_INTROSPECTION:
    case HwAsic::Feature::SAI_MPLS_QOS:
    case HwAsic::Feature::QOS_MAP_GLOBAL:
    case HwAsic::Feature::MACSEC:
    case HwAsic::Feature::SAI_PORT_GET_PMD_LANES:
    case HwAsic::Feature::SAI_PORT_VCO_CHANGE:
    case HwAsic::Feature::ROUTE_METADATA:
    case HwAsic::Feature::DLB:
    case HwAsic::Feature::IN_PAUSE_INCREMENTS_DISCARDS:
    case HwAsic::Feature::WARMBOOT:
    case HwAsic::Feature::SAI_CONFIGURE_SIX_TAP:
    case HwAsic::Feature::UDF_HASH_FIELD_QUERY:
    case HwAsic::Feature::SAI_SAMPLEPACKET_TRAP:
    case HwAsic::Feature::TRAP_PRIORITY_LOWER_VAL_IS_LOWER_PRI:
    case HwAsic::Feature::NON_UNICAST_HASH:
    case HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER:
    case HwAsic::Feature::SAI_UDF_HASH:
    case HwAsic::Feature::INGRESS_PRIORITY_GROUP_HEADROOM_WATERMARK:
      return true;
    // features not working well with bcmsim
    case HwAsic::Feature::MIRROR_PACKET_TRUNCATION:
    case HwAsic::Feature::SFLOW_SAMPLING:
    case HwAsic::Feature::PFC:
      return getAsicMode() != AsicMode::ASIC_MODE_SIM;

    case HwAsic::Feature::QCM:
    case HwAsic::Feature::SMAC_EQUALS_DMAC_CHECK_ENABLED:
    case HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::PORT_INTERFACE_TYPE:
    case HwAsic::Feature::HOSTTABLE:
    case HwAsic::Feature::PORT_TX_DISABLE:
    case HwAsic::Feature::ZERO_SDK_WRITE_WARMBOOT:
    case HwAsic::Feature::PENDING_L2_ENTRY:
    case HwAsic::Feature::COUNTER_REFRESH_INTERVAL:
    case HwAsic::Feature::WIDE_ECMP:
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT: // CS00012066057
    case HwAsic::Feature::EMPTY_ACL_MATCHER:
    case HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET:
    case HwAsic::Feature::SAI_ACL_TABLE_UPDATE:
    case HwAsic::Feature::PORT_EYE_VALUES:
    case HwAsic::Feature::SAI_MPLS_TTL_1_TRAP:
    case HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER:
    case HwAsic::Feature::SAI_PORT_ERR_STATUS:
    case HwAsic::Feature::RX_FREQUENCY_PPM:
    case HwAsic::Feature::FABRIC_PORTS:
    case HwAsic::Feature::FABRIC_PORT_MTU:
    case HwAsic::Feature::SAI_FIRMWARE_PATH:
    case HwAsic::Feature::EXTENDED_FEC:
    case HwAsic::Feature::LINK_TRAINING:
    case HwAsic::Feature::SAI_RX_REASON_COUNTER:
    case HwAsic::Feature::SAI_MPLS_INSEGMENT:
    case HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE:
    case HwAsic::Feature::VOQ:
    case HwAsic::Feature::RECYCLE_PORTS:
    case HwAsic::Feature::XPHY_PORT_STATE_TOGGLE:
    case HwAsic::Feature::FABRIC_TX_QUEUES:
    case HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE:
    case HwAsic::Feature::SHARED_INGRESS_EGRESS_BUFFER_POOL:
    case HwAsic::Feature::P4_WARMBOOT:
    case HwAsic::Feature::FEC_AM_LOCK_STATUS:
    case HwAsic::Feature::PCS_RX_LINK_STATUS:
    case HwAsic::Feature::TC_TO_QUEUE_QOS_MAP_ON_SYSTEM_PORT:
    case HwAsic::Feature::PORT_FABRIC_ISOLATE:
    case HwAsic::Feature::QUEUE_ECN_COUNTER:
    case HwAsic::Feature::CPU_TX_VIA_RECYCLE_PORT:
    case HwAsic::Feature::QUEUE_PRIORITY_LOWER_VAL_IS_HIGH_PRI:
    case HwAsic::Feature::SAI_CONFIGURE_SEVEN_TAP:
    case HwAsic::Feature::SWITCH_DROP_STATS:
    case HwAsic::Feature::RX_LANE_SQUELCH_ENABLE:
    case HwAsic::Feature::SAI_PORT_ETHER_STATS:
      return false;
  }
  return false;
}

int Tomahawk4Asic::getDefaultACLGroupID() const {
  if (FLAGS_acl_gid > 0) {
    return FLAGS_acl_gid;
  } else {
    return kDefaultACLGroupID;
  }
}

int Tomahawk4Asic::getDefaultTeFlowGroupID() const {
  if (FLAGS_teFlow_gid > 0) {
    return FLAGS_teFlow_gid;
  } else {
    return kDefaultTeFlowGroupID;
  }
}

int Tomahawk4Asic::getStationID(int intfId) const {
  int stationId = intfId;
  // station id should be smaller than 511 on tomahawk4
  if (intfId >= 4000) {
    stationId = intfId - 4000 + 400; // 400, 401, 402, ...
  } else if (intfId >= 3000) {
    // <RGSW, FUJI> in OLR has 96 uplinks hence we cannot use 4000
    stationId = intfId - 3000 + 300; // 300, 301, 302, ...
  } else if (intfId >= 2000) {
    stationId = intfId - 2000 + 200; // 200, 201, 202, ...
  } else if (intfId >= 1000) {
    // kBaseVlanId used in ConfigFactory for testing purpose is 1000
    stationId = intfId - 1000 + 100; // 100, 101, 102, ...
  }
  return stationId;
}

int Tomahawk4Asic::getNumLanesPerPhysicalPort() const {
  /*
    In each Blackhawk7 core, there are 4 phyiscal ports and (up to) 4 logical
    ports but 8 physical lanes. Therefore, when calculating the physical_port of
    bcm_port_resource_t when using flexing port logic, we need to use
    numLanesPerPhysicalPort to divide physical lanes, which is learned from
    PlatformMapping.
  */
  return 2;
}

int Tomahawk4Asic::getDefaultNumPortQueues(cfg::StreamType streamType, bool cpu)
    const {
  // 12 logical queues in total, same as tomahawk3
  switch (streamType) {
    case cfg::StreamType::UNICAST:
      if (cpu) {
        break;
      }
      return 8;
    case cfg::StreamType::MULTICAST:
      return cpu ? 10 : 4;
    case cfg::StreamType::ALL:
    case cfg::StreamType::FABRIC_TX:
      break;
  }
  throw FbossError(
      "Unexpected, stream: ", streamType, " cpu: ", cpu, "combination");
}
} // namespace facebook::fboss
