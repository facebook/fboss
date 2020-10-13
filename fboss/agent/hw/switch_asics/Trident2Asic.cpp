// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/Trident2Asic.h"

namespace facebook::fboss {

bool Trident2Asic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::HOSTTABLE_FOR_HOSTROUTES:
    case HwAsic::Feature::SPAN:
    case HwAsic::Feature::ERSPANv4:
    case HwAsic::Feature::HOT_SWAP:
    case HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION:
    case HwAsic::Feature::QUEUE:
    case HwAsic::Feature::WARM_BOOT:
    case HwAsic::Feature::ROUTE_METADATA:
    case HwAsic::Feature::SCHEDULER_PPS:
    case HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::PORT_INTERFACE_TYPE:
    case HwAsic::Feature::L2ENTRY_METADATA:
    case HwAsic::Feature::NEIGHBOR_METADATA:
    case HwAsic::Feature::DEBUG_COUNTER:
    case HwAsic::Feature::RESOURCE_USAGE_STATS:
    case HwAsic::Feature::OBJECT_KEY_CACHE:
    case HwAsic::Feature::ACL_COPY_TO_CPU:
    case HwAsic::Feature::BUFFER_PROFILE:
      return true;

    case HwAsic::Feature::TRUNCATE_MIRROR_PACKET:
    case HwAsic::Feature::ERSPANv6:
    case HwAsic::Feature::SFLOWv4:
    case HwAsic::Feature::SFLOWv6:
    case HwAsic::Feature::MPLS:
    case HwAsic::Feature::MPLS_ECMP:
    case HwAsic::Feature::TX_VLAN_STRIPPING_ON_PORT:
    case HwAsic::Feature::ECN:
    case HwAsic::Feature::L3_QOS:
    case HwAsic::Feature::QOS_MAP_GLOBAL:
    case HwAsic::Feature::QCM:
    case HwAsic::Feature::SMAC_EQUALS_DMAC_CHECK_ENABLED:
    case HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER:
    case HwAsic::Feature::HSDK:
    case HwAsic::Feature::L3_EGRESS_MODE_AUTO_ENABLED:
    case HwAsic::Feature::SAI_ECN_WRED:
    case HwAsic::Feature::PKTIO:
      return false;
  }
  return false;
}

} // namespace facebook::fboss
