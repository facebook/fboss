// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"

namespace facebook::fboss {

bool TomahawkAsic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::SPAN:
    case HwAsic::Feature::ERSPANv4:
    case HwAsic::Feature::SFLOWv4:
    case HwAsic::Feature::MPLS:
    case HwAsic::Feature::MPLS_ECMP:
    case HwAsic::Feature::HOT_SWAP:
    case HwAsic::Feature::PORT_PREEMPHASIS:
    case HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION:
    case HwAsic::Feature::QUEUE:
    case HwAsic::Feature::ECN:
    case HwAsic::Feature::L3_QOS:
      return true;

    case HwAsic::Feature::HOSTTABLE_FOR_HOSTROUTES:
    case HwAsic::Feature::TRUNCATE_MIRROR_PACKET:
    case HwAsic::Feature::ERSPANv6:
    case HwAsic::Feature::SFLOWv6:
    case HwAsic::Feature::TX_VLAN_STRIPPING_ON_PORT:
      return false;
  }
  return false;
}

} // namespace facebook::fboss
