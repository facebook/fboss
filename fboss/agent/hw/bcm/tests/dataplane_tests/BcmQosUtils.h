/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <boost/container/flat_map.hpp>
#include <folly/IPAddress.h>
#include <cstdint>

#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/state/Route.h"

extern "C" {
#include <bcm/types.h>
}

class BcmSwitch;

namespace facebook::fboss::utility {

/*
 * Given a specific port, BcmCosToQueueMapper can provide the queue to which a
 * class-of-service is mapped to.
 */
class BcmCosToQueueMapper {
 public:
  explicit BcmCosToQueueMapper(bcm_gport_t gport);

  bcm_gport_t getUcastQueueForCos(uint8_t cos);
  bcm_gport_t getMcastQueueForCos(uint8_t cos);

 private:
  void discoverCosToQueueMapping();
  uint8_t getNextUcastCosID();
  uint8_t getNextMcastCosID();
  static int gportTraverseCallback(
      int unit,
      bcm_gport_t gport,
      int numq,
      uint32 flags,
      bcm_gport_t queue_or_sched_gport,
      void* user_data);

  using CosToQueueGportMap = boost::container::flat_map<uint8_t, bcm_gport_t>;

  bcm_gport_t gport_;
  CosToQueueGportMap cosToUcastQueueMap_;
  CosToQueueGportMap cosToMcastQueueMap_;
  uint8_t nextUcastCosID_{0};
  uint8_t nextMcastCosID_{0};
  bool initialized_{false};
};

} // namespace facebook::fboss::utility
