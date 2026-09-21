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
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thread>

using namespace facebook::fboss;

namespace facebook::fboss::utility {
namespace {

bool queueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort) {
  auto queuePacketsBefore =
      portStatsBefore.queueOutPackets_()->find(queueId)->second;
  auto portStatsAfter = ensemble->getLatestPortStats(egressPort);
  auto queuePacketsAfter = portStatsAfter.queueOutPackets_()[queueId];
  // Note, on some platforms, due to how loopbacked packets are pruned
  // from being broadcast, they will appear more than once on a queue
  // counter, so we can only check that the counter went up, not that it
  // went up by exactly one.
  XLOG(DBG2) << "Port ID: " << egressPort << " queue: " << queueId
             << " queuePacketsBefore " << queuePacketsBefore
             << " queuePacketsAfter " << queuePacketsAfter;
  return queuePacketsAfter > queuePacketsBefore;
}

bool voqHit(
    const HwSysPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::SystemPortID egressPort) {
  auto queueBytesBefore =
      portStatsBefore.queueOutBytes_()->find(queueId)->second;
  auto portStatsAfter = ensemble->getLatestSysPortStats(egressPort);
  auto queueBytesAfter = portStatsAfter.queueOutBytes_()[queueId];
  XLOG(DBG2) << "Sys port: " << egressPort << " queue " << queueId
             << " queueBytesBefore " << queueBytesBefore << " queueBytesAfter "
             << queueBytesAfter;
  return queueBytesAfter > queueBytesBefore;
}
} // namespace

void verifyQueueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort) {
  WITH_RETRIES({
    EXPECT_EVENTUALLY_TRUE(
        queueHit(portStatsBefore, queueId, ensemble, egressPort));
  });
}

void verifyVoQHit(
    const HwSysPortStats& portStatsBefore,
    int queueId,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::SystemPortID egressPort) {
  WITH_RETRIES({
    EXPECT_EVENTUALLY_TRUE(
        voqHit(portStatsBefore, queueId, ensemble, egressPort));
  });
}

} // namespace facebook::fboss::utility
