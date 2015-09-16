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

extern "C" {
#include <opennsl/types.h>
#include <opennsl/stat.h>
}

#include "common/stats/MonotonicCounter.h"
#include "common/stats/ExportedHistogram.h"
#include "fboss/agent/types.h"

#include <mutex>

namespace facebook { namespace fboss {

class BcmPlatformPort;
class BcmSwitch;

/**
 * BcmPort is the class to abstract the physical port in BcmSwitch.
 */
class BcmPort {
 public:
  /*
   * Construct the BcmPort object.
   *
   * This method shouldn't make any calls to the Broadcom SDK to query/modify
   * the port yet.  init() will be called soon after construction, and any
   * actual initialization logic should be performed there.
   */
  BcmPort(BcmSwitch* hw, opennsl_port_t port, BcmPlatformPort* platformPort);

  void init(bool warmBoot);

  /*
   * Getters.
   */
  BcmPlatformPort* getPlatformPort() const {
    return platformPort_;
  }
  BcmSwitch* getHW() const {
    return hw_;
  }
  opennsl_port_t getBcmPortId() const {
    return port_;
  }
  opennsl_gport_t getBcmGport() const {
    return gport_;
  }

  /*
   * Setters.
   */
  void setPortStatus(int status);

  /*
   * Update this port's statistics.
   */
  void updateStats();

 private:
  class MonotonicCounter : public stats::MonotonicCounter {
   public:
    // Inherit stats::MonotonicCounter constructors
    using stats::MonotonicCounter::MonotonicCounter;

    // Export SUM and RATE by default
    explicit MonotonicCounter(const std::string& name)
      : stats::MonotonicCounter(name, stats::SUM, stats::RATE) {}
  };

  // no copy or assignment
  BcmPort(BcmPort const &) = delete;
  BcmPort& operator=(BcmPort const &) = delete;

  void updateStat(std::chrono::seconds now,
                  stats::MonotonicCounter* stat,
                  opennsl_stat_val_t type);
  void updatePktLenHist(std::chrono::seconds now,
                        stats::ExportedHistogramMap::LockAndHistogram* hist,
                        const std::vector<opennsl_stat_val_t>& stats);
  std::string statName(folly::StringPiece name) const;

  void disablePause();
  void setAdditionalStats(std::chrono::seconds now);

  BcmSwitch* const hw_{nullptr};
  const opennsl_port_t port_;    // Broadcom physical port number
  opennsl_gport_t gport_;  // Broadcom global port number
  BcmPlatformPort* const platformPort_{nullptr};

  MonotonicCounter inBytes_{statName("in_bytes")};
  MonotonicCounter inUnicastPkts_{statName("in_unicast_pkts")};
  MonotonicCounter inMulticastPkts_{statName("in_multicast_pkts")};
  MonotonicCounter inBroadcastPkts_{statName("in_broadcast_pkts")};
  MonotonicCounter inDiscards_{statName("in_discards")};
  MonotonicCounter inErrors_{statName("in_errors")};
  MonotonicCounter inPause_{statName("in_pause_frames")};

  MonotonicCounter outBytes_{statName("out_bytes")};
  MonotonicCounter outUnicastPkts_{statName("out_unicast_pkts")};
  MonotonicCounter outMulticastPkts_{statName("out_multicast_pkts")};
  MonotonicCounter outBroadcastPkts_{statName("out_broadcast_pkts")};
  MonotonicCounter outDiscards_{statName("out_discards")};
  MonotonicCounter outErrors_{statName("out_errors")};
  MonotonicCounter outPause_{statName("out_pause_frames")};

  stats::ExportedStatMap::LockAndStatItem outQueueLen_;
  stats::ExportedHistogramMap::LockAndHistogram inPktLengths_;
  stats::ExportedHistogramMap::LockAndHistogram outPktLengths_;
};

}} // namespace facebook::fboss
