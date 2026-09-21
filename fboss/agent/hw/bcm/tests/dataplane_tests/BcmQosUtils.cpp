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
} // namespace facebook::fboss::utility
