// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmQosUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <bcm/qos.h>
}

namespace {
// array with index representing the traffic class and array value is priority
// group (PG) defaults from HW required for mapping incoming traffic to PG on
// ingress for PFC purposes
const std::vector<int>
    kDefaultTrafficClassToPg{0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7};
// lossless mode has different defaults
const std::vector<int> kDefaultTrafficClassToPgInLosslessMode{
    7,
    1,
    2,
    3,
    4,
    5,
    6,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0};
// array with index representing pfc priority and value is priority group (PG)
// defaults from HW
const std::vector<int> kDefaultPfcPriorityToPg{0, 0, 0, 0, 0, 0, 0, 0};
constexpr int kDefaultProfileId = 0;
const std::vector<int> kDefaultPfcEnableForPriToQueue =
    {1, 1, 1, 1, 1, 1, 1, 1};
const std::vector<int> kDefaultPfcOptimizedForPriToQueue =
    {0, 0, 0, 0, 0, 0, 0, 0};
} // unnamed namespace

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

const std::vector<int>& getDefaultTrafficClassToPg() {
  return kDefaultTrafficClassToPg;
}

const std::vector<int>& getDefaultPfcPriorityToPg() {
  return kDefaultPfcPriorityToPg;
}

int getDefaultProfileId() {
  return kDefaultProfileId;
}

const std::vector<int>& getDefaultPfcEnablePriorityToQueue() {
  return kDefaultPfcEnableForPriToQueue;
}

const std::vector<int>& getDefaultPfcOptimizedPriorityToQueue() {
  return kDefaultPfcOptimizedForPriToQueue;
}

} // namespace facebook::fboss
