/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmStats.h"

#include "fboss/agent/SwitchStats.h"
#include "common/stats/ExportedStatMapImpl.h"

using facebook::stats::SUM;
using facebook::stats::RATE;

namespace facebook { namespace fboss {

folly::ThreadLocalPtr<BcmStats> BcmStats::stats_;

BcmStats::BcmStats()
    : BcmStats(stats::ThreadCachedServiceData::get()->getThreadStats()) {
}

BcmStats::BcmStats(ThreadLocalStatsMap *map)
    : txPktAlloc_(map, SwitchStats::kCounterPrefix + "bcm.tx.pkt.allocated",
                  SUM, RATE),
      txPktFree_(map, SwitchStats::kCounterPrefix + "bcm.tx.pkt.freed",
                 SUM, RATE),
      txSent_(map, SwitchStats::kCounterPrefix + "bcm.tx.pkt.sent",
              SUM, RATE),
      txSentDone_(map, SwitchStats::kCounterPrefix + "bcm.tx.pkt.sent.done",
                  SUM, RATE),
      txErrors_(map, SwitchStats::kCounterPrefix + "bcm.tx.errors",
                  SUM, RATE),
      txPktAllocErrors_(map, SwitchStats::kCounterPrefix +
          "bcm.tx.pkt.allocation.errors", SUM, RATE),
      txQueued_(map, SwitchStats::kCounterPrefix + "bcm.tx.pkt.queued_us",
                100, 0, 1000),
      parityErrors_(map, SwitchStats::kCounterPrefix + "bcm.parity.errors",
                    SUM, RATE) {
}

BcmStats* BcmStats::createThreadStats() {
  BcmStats* s = new BcmStats();
  stats_.reset(s);
  return s;
}

}}
