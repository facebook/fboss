/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

extern "C" {
#include <bcm/cosq.h>
#include <bcm/error.h>
#include <bcm/l3.h>
}

namespace facebook::fboss::utility {

BcmCosToQueueMapper::BcmCosToQueueMapper(bcm_gport_t gport)
    : gport_(gport), cosToUcastQueueMap_(), cosToMcastQueueMap_() {}

bcm_gport_t BcmCosToQueueMapper::getUcastQueueForCos(uint8_t cos) {
  if (!initialized_) {
    discoverCosToQueueMapping();
  }

  auto it = cosToUcastQueueMap_.find(cos);
  CHECK(it != cosToUcastQueueMap_.end());

  return it->second;
}

bcm_gport_t BcmCosToQueueMapper::getMcastQueueForCos(uint8_t cos) {
  if (!initialized_) {
    discoverCosToQueueMapping();
  }

  auto it = cosToMcastQueueMap_.find(cos);
  CHECK(it != cosToMcastQueueMap_.end());

  return it->second;
}

void BcmCosToQueueMapper::discoverCosToQueueMapping() {
  auto rv = bcm_cosq_gport_traverse(
      0, &BcmCosToQueueMapper::gportTraverseCallback, this);
  bcmCheckError(rv, "failed to traverse queues for gport ", gport_);

  initialized_ = true;
}

uint8_t BcmCosToQueueMapper::getNextUcastCosID() {
  return nextUcastCosID_++;
}

uint8_t BcmCosToQueueMapper::getNextMcastCosID() {
  return nextMcastCosID_++;
}

int BcmCosToQueueMapper::gportTraverseCallback(
    int /* unit */,
    bcm_gport_t gport,
    int /* numq */,
    uint32 flags,
    bcm_gport_t queue_or_sched_gport,
    void* user_data) {
  auto mapper = static_cast<BcmCosToQueueMapper*>(user_data);

  /*
   * To break out of the traversal with a Broadcom-native error code requires
   * having #define'd CB_ABORT_ON_ERR. Because `inserted` may only evaluate
   * to false on an FBOSS-side logic error, we fail hard if emplace fails to
   * insert successfully.
   */
  bool inserted = false;
  if (mapper->gport_ == gport) {
    if (flags == BCM_COSQ_GPORT_UCAST_QUEUE_GROUP) {
      std::tie(std::ignore, inserted) = mapper->cosToUcastQueueMap_.emplace(
          mapper->getNextUcastCosID(), queue_or_sched_gport);
      CHECK(inserted);
    } else if (flags == BCM_COSQ_GPORT_MCAST_QUEUE_GROUP) {
      std::tie(std::ignore, inserted) = mapper->cosToMcastQueueMap_.emplace(
          mapper->getNextMcastCosID(), queue_or_sched_gport);
      CHECK(inserted);
    }
  }

  return BCM_E_NONE;
}

void disableTTLDecrements(
    BcmSwitch* hw,
    RouterID routerId,
    const folly::IPAddress& addr) {
  auto vrfId = hw->getBcmVrfId(routerId);
  auto bcmHostKey = BcmHostKey(vrfId, addr);
  const auto hostTable = hw->getHostTable();
  auto bcmHost = hostTable->getBcmHostIf(bcmHostKey);
  CHECK(bcmHost) << "failed to find host for " << bcmHostKey.str();

  bcm_if_t id = bcmHost->getEgressId();
  bcm_l3_egress_t egr;
  auto rv = bcm_l3_egress_get(hw->getUnit(), bcmHost->getEgressId(), &egr);
  bcmCheckError(rv, "failed bcm_l3_egress_get");

  // Opennsl does not define a flag for BCM_L3_KEEP_TTL
  uint32_t flags = BCM_L3_REPLACE | BCM_L3_WITH_ID | BCM_L3_KEEP_TTL;
  rv = bcm_l3_egress_create(hw->getUnit(), flags, &egr, &id);
  bcmCheckError(rv, "failed bcm_l3_egress_create");
}

} // namespace facebook::fboss::utility
