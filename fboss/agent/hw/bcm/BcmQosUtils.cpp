// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmQosUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <bcm/qos.h>
}

namespace facebook::fboss {

std::vector<BcmQosMapIdAndFlag> getBcmQosMapIdsAndFlags(int unit) {
  int numEntries;
  auto rv = bcm_qos_multi_get(unit, 0, nullptr, nullptr, &numEntries);
  bcmCheckError(rv, "Unable to get the number of QoS maps");

  std::vector<int> mapIds(numEntries);
  std::vector<int> flags(numEntries);
  rv = bcm_qos_multi_get(
      unit, numEntries, mapIds.data(), flags.data(), &numEntries);
  bcmCheckError(rv, "Unable to get the QoS maps");

  std::vector<BcmQosMapIdAndFlag> mapIdsAndFlags(numEntries);
  for (int i = 0; i < numEntries; ++i) {
    mapIdsAndFlags[i] = std::make_pair(mapIds[i], flags[i]);
  }
  return mapIdsAndFlags;
}

int getBcmQosMapFlagsL3Ingress() {
  return BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3;
}

int getBcmQosMapFlagsL3Egress() {
  return BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_L3;
}

int getBcmQosMapFlagsMPLSIngress() {
  return BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_MPLS;
}

int getBcmQosMapFlagsMPLSEgress() {
  return BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_MPLS;
}

} // namespace facebook::fboss
