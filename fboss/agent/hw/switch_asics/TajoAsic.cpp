// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/TajoAsic.h"

namespace facebook::fboss {

bool TajoAsic::isSupported(Feature feature) const {
  switch (feature) {
    /*
     * Except TX_VLAN_STRIPPING_ON_PORT, none of the other features are
     * verified on the asic. Marking them as true for now but need to revisit
     * this as we verify the features.
     */
    case HwAsic::Feature::SPAN:
    case HwAsic::Feature::ERSPANv4:
    case HwAsic::Feature::SFLOWv4:
    case HwAsic::Feature::MPLS:
    case HwAsic::Feature::MPLS_ECMP:
    case HwAsic::Feature::TRUNCATE_MIRROR_PACKET:
    case HwAsic::Feature::ERSPANv6:
    case HwAsic::Feature::SFLOWv6:
    case HwAsic::Feature::HOSTTABLE_FOR_HOSTROUTES:
    case HwAsic::Feature::TX_VLAN_STRIPPING_ON_PORT:
    case HwAsic::Feature::ECN:
    case HwAsic::Feature::L3_QOS:
      return true;
    case HwAsic::Feature::HOT_SWAP:
    case HwAsic::Feature::PORT_PREEMPHASIS:
    case HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION:
    case HwAsic::Feature::QUEUE:
    case HwAsic::Feature::WARM_BOOT:
      return false;
  }
  return false;
}

} // namespace facebook::fboss
