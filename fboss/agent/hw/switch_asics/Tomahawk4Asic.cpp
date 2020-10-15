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

constexpr auto kDefaultDropEgressID = 100001;
} // namespace

namespace facebook::fboss {

bool Tomahawk4Asic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::SPAN:
    case HwAsic::Feature::ERSPANv4:
    case HwAsic::Feature::SFLOWv4:
    case HwAsic::Feature::MPLS:
    case HwAsic::Feature::MPLS_ECMP:
    case HwAsic::Feature::TRUNCATE_MIRROR_PACKET:
    case HwAsic::Feature::ERSPANv6:
    case HwAsic::Feature::SFLOWv6:
    case HwAsic::Feature::HOT_SWAP:
    case HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION:
    case HwAsic::Feature::QUEUE:
    case HwAsic::Feature::ECN:
    case HwAsic::Feature::L3_QOS:
    case HwAsic::Feature::WARM_BOOT:
    case HwAsic::Feature::ROUTE_METADATA:
    case HwAsic::Feature::SCHEDULER_PPS:
    case HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::L2ENTRY_METADATA:
    case HwAsic::Feature::NEIGHBOR_METADATA:
    case HwAsic::Feature::DEBUG_COUNTER:
    case HwAsic::Feature::RESOURCE_USAGE_STATS:
    case HwAsic::Feature::HSDK:
    case HwAsic::Feature::OBJECT_KEY_CACHE:
    case HwAsic::Feature::L3_EGRESS_MODE_AUTO_ENABLED:
    case HwAsic::Feature::PKTIO:
    case HwAsic::Feature::ACL_COPY_TO_CPU:
    case HwAsic::Feature::BUFFER_PROFILE:
    case HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER:
      return true;

    case HwAsic::Feature::HOSTTABLE_FOR_HOSTROUTES:
    case HwAsic::Feature::TX_VLAN_STRIPPING_ON_PORT:
    case HwAsic::Feature::QOS_MAP_GLOBAL:
    case HwAsic::Feature::QCM:
    case HwAsic::Feature::SMAC_EQUALS_DMAC_CHECK_ENABLED:
    case HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE:
    case HwAsic::Feature::PORT_INTERFACE_TYPE:
    case HwAsic::Feature::SAI_ECN_WRED:
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

int Tomahawk4Asic::getStationID(int intfId) const {
  int stationId = intfId;
  // station id should be smaller than 511 on tomahawk4
  if (intfId >= 4000) {
    stationId = intfId - 4000 + 400; // 400, 401, 402
  } else if (intfId >= 2000) {
    stationId = intfId - 2000 + 200; // 200, 201, 202, ...
  }
  return stationId;
}

int Tomahawk4Asic::getDefaultDropEgressID() const {
  return kDefaultDropEgressID;
}
} // namespace facebook::fboss
