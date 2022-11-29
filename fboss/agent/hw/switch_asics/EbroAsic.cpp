// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {

bool EbroAsic::isSupportedNonFabric(Feature feature) const {
  switch (feature) {
    /*
     * None of the features are verified on the asic.
     * Marking them as true for now but need to revisit
     * this as we verify the features.
     */
    case HwAsic::Feature::SPAN:
    case HwAsic::Feature::ERSPANv4:
    case HwAsic::Feature::SFLOWv4:
    case HwAsic::Feature::MPLS:
    case HwAsic::Feature::MPLS_ECMP:
    case HwAsic::Feature::ERSPANv6:
    case HwAsic::Feature::SFLOWv6:
    case HwAsic::Feature::HOSTTABLE_FOR_HOSTROUTES:
    case HwAsic::Feature::ECN:
    case HwAsic::Feature::L3_QOS:
    case HwAsic::Feature::QOS_MAP_GLOBAL:
    case HwAsic::Feature::SMAC_EQUALS_DMAC_CHECK_ENABLED:
    case HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER:
    case HwAsic::Feature::SAI_WEIGHTED_NEXTHOPGROUP_MEMBER:
    case HwAsic::Feature::SWITCH_ATTR_INGRESS_ACL:
    case HwAsic::Feature::ACL_COPY_TO_CPU:
    case HwAsic::Feature::BUFFER_POOL:
    case HwAsic::Feature::MIRROR_PACKET_TRUNCATION:
    case HwAsic::Feature::SFLOW_SAMPLING:
    case HwAsic::Feature::SAI_ECN_WRED:
    case HwAsic::Feature::RESOURCE_USAGE_STATS:
    case HwAsic::Feature::DEBUG_COUNTER:
    case HwAsic::Feature::PENDING_L2_ENTRY:
    case HwAsic::Feature::COUNTER_REFRESH_INTERVAL:
    case HwAsic::Feature::MAC_AGING:
    case HwAsic::Feature::TELEMETRY_AND_MONITORING:
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT:
    case HwAsic::Feature::DEFAULT_VLAN:
    case HwAsic::Feature::L2_LEARNING:
    case HwAsic::Feature::TRAFFIC_HASHING:
    case HwAsic::Feature::ACL_TABLE_GROUP:
    case HwAsic::Feature::CPU_PORT:
    case HwAsic::Feature::VRF:
    case HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET:
    case HwAsic::Feature::PTP_TC:
    case HwAsic::Feature::PTP_TC_PCS:
    case HwAsic::Feature::SAI_FEC_COUNTERS:
    case HwAsic::Feature::ROUTE_PROGRAMMING:
    case HwAsic::Feature::ECMP_HASH_V4:
    case HwAsic::Feature::ECMP_HASH_V6:
    case HwAsic::Feature::MEDIA_TYPE:
    case HwAsic::Feature::FEC:
    case HwAsic::Feature::ECMP_MEMBER_WIDTH_INTROSPECTION:
    case HwAsic::Feature::FABRIC_PORT_MTU:
    case HwAsic::Feature::SAI_MPLS_INSEGMENT:
    case HwAsic::Feature::FABRIC_PORTS:
      return true;
    // VOQ vs NPU mode dependent features
    case HwAsic::Feature::BRIDGE_PORT_8021Q:
      return getSwitchType() == cfg::SwitchType::NPU;
    case HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE:
      return getSwitchType() == cfg::SwitchType::VOQ;
    case HwAsic::Feature::HOSTTABLE:
    case HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION:
    case HwAsic::Feature::QCM:
    case HwAsic::Feature::SCHEDULER_PPS:
    case HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::PORT_INTERFACE_TYPE:
    case HwAsic::Feature::HSDK:
    case HwAsic::Feature::OBJECT_KEY_CACHE:
    case HwAsic::Feature::L3_EGRESS_MODE_AUTO_ENABLED:
    case HwAsic::Feature::PKTIO:
    case HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER:
    case HwAsic::Feature::PORT_TX_DISABLE:
    case HwAsic::Feature::ZERO_SDK_WRITE_WARMBOOT:
    case HwAsic::Feature::OBM_COUNTERS:
    case HwAsic::Feature::EGRESS_QUEUE_FLEX_COUNTER:
    case HwAsic::Feature::PFC:
    case HwAsic::Feature::INGRESS_L3_INTERFACE:
    case HwAsic::Feature::NON_UNICAST_HASH:
    case HwAsic::Feature::DETAILED_L2_UPDATE:
    case HwAsic::Feature::WIDE_ECMP:
    case HwAsic::Feature::ALPM_ROUTE_PROJECTION:
    case HwAsic::Feature::SAI_PORT_SPEED_CHANGE:
    case HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD:
    case HwAsic::Feature::EGRESS_MIRRORING:
    case HwAsic::Feature::EGRESS_SFLOW:
    case HwAsic::Feature::SAI_LAG_HASH:
    case HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER:
    case HwAsic::Feature::MACSEC:
    case HwAsic::Feature::SAI_HASH_FIELDS_CLEAR_BEFORE_SET:
    case HwAsic::Feature::SAI_MPLS_QOS:
    case HwAsic::Feature::EMPTY_ACL_MATCHER:
    case HwAsic::Feature::ROUTE_COUNTERS:
    case HwAsic::Feature::MULTIPLE_ACL_TABLES:
    case HwAsic::Feature::ROUTE_FLEX_COUNTERS:
    case HwAsic::Feature::FEC_DIAG_COUNTERS:
    case HwAsic::Feature::SAI_ACL_TABLE_UPDATE:
    case HwAsic::Feature::PORT_EYE_VALUES:
    case HwAsic::Feature::SAI_MPLS_TTL_1_TRAP:
    case HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER:
    case HwAsic::Feature::PMD_RX_LOCK_STATUS:
    case HwAsic::Feature::PMD_RX_SIGNAL_DETECT:
    case HwAsic::Feature::SAI_PORT_ERR_STATUS:
    case HwAsic::Feature::EXACT_MATCH:
    case HwAsic::Feature::FEC_CORRECTED_BITS:
    case HwAsic::Feature::RX_FREQUENCY_PPM:
    case HwAsic::Feature::SAI_FIRMWARE_PATH:
    case HwAsic::Feature::EXTENDED_FEC:
    case HwAsic::Feature::LINK_TRAINING:
    case HwAsic::Feature::SAI_RX_REASON_COUNTER:
    case HwAsic::Feature::VOQ:
    case HwAsic::Feature::RECYCLE_PORTS:
    case HwAsic::Feature::XPHY_PORT_STATE_TOGGLE:
    case HwAsic::Feature::SAI_PORT_GET_PMD_LANES:
    case HwAsic::Feature::FABRIC_TX_QUEUES:
    case HwAsic::Feature::SAI_PORT_VCO_CHANGE:
    case HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE:
      return false;
  }
  return false;
}

bool EbroAsic::isSupportedFabric(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT:
    case HwAsic::Feature::FABRIC_PORTS:
      return true;
    default:
      return false;
  }
  return false;
}

int EbroAsic::getDefaultNumPortQueues(cfg::StreamType streamType, bool /*cpu*/)
    const {
  switch (streamType) {
    case cfg::StreamType::MULTICAST:
      throw FbossError(
          "no queue exist for stream type: ",
          apache::thrift::util::enumNameSafe(streamType));
    case cfg::StreamType::FABRIC_TX:
      return 1;
    case cfg::StreamType::UNICAST:
    case cfg::StreamType::ALL:
      return 8;
  }

  throw FbossError("Unknown streamType", streamType);
}
std::set<cfg::StreamType> EbroAsic::getQueueStreamTypes(
    cfg::PortType portType) const {
  switch (portType) {
    case cfg::PortType::CPU_PORT:
    case cfg::PortType::INTERFACE_PORT:
      /*
       * For Ebro asic, SDK 1.42.* queue type is ALL whereas SDK 1.56.* queue
       * type is unicast. Once SDK 1.56.* is rolled out after P4 warmboot
       * support across the entire fleet, the stream type can be encoded as
       * Unicast.
       */
      return {getDefaultStreamType()};
    case cfg::PortType::FABRIC_PORT:
      return {cfg::StreamType::FABRIC_TX};
    case cfg::PortType::RECYCLE_PORT:
      // TODO: handle when we start modeling
      // recycle port for Ebro ASIC
      break;
  }
  throw FbossError(
      "Ebro ASIC does not support:",
      apache::thrift::util::enumNameSafe(portType));
}
cfg::Range64 EbroAsic::getReservedEncapIndexRange() const {
  if (getSwitchType() == cfg::SwitchType::VOQ) {
    // Reserved range worked out with vendor. These ids
    // are reserved in SAI-SDK implementation for use
    // by NOS
    return makeRange(100, 4093);
  }
  return HwAsic::getReservedEncapIndexRange();
}
} // namespace facebook::fboss
