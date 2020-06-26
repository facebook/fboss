/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include <folly/logging/xlog.h>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace facebook::fboss::utility {

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort) {
  auto retries = 10;
  bool statsMatch;
  do {
    auto portStatsAfter = ensemble->getLatestPortStats(egressPort);
    statsMatch = true;
    for (const auto& _q2dscps : q2dscps) {
      auto queuePacketsBefore =
          portStatsBefore.queueOutPackets__ref()->find(_q2dscps.first)->second;
      auto queuePacketsAfter =
          portStatsAfter.queueOutPackets__ref()[_q2dscps.first];
      // Note, on some platforms, due to how loopbacked packets are pruned
      // from being broadcast, they will appear more than once on a queue
      // counter, so we can only check that the counter went up, not that it
      // went up by exactly one.
      if (queuePacketsAfter < queuePacketsBefore + _q2dscps.second.size()) {
        statsMatch = false;
        break;
      }
    }
    if (!statsMatch) {
      std::this_thread::sleep_for(20ms);
    } else {
      break;
    }
    XLOG(INFO) << " Retrying ...";
  } while (--retries && !statsMatch);
  return statsMatch;
}
} // namespace facebook::fboss::utility
