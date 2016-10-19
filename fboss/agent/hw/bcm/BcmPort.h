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
#include <opennsl/port.h>
}

#include "common/stats/MonotonicCounter.h"
#include "common/stats/ExportedStatMapImpl.h"
#include "common/stats/ExportedHistogramMapImpl.h"
#include "fboss/agent/types.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/gen-cpp/switch_config_types.h"

#include <mutex>

namespace facebook { namespace fboss {

class BcmSwitch;
class BcmPortGroup;
class SwitchState;
class Port;

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

  void enable(const std::shared_ptr<Port>& swPort);
  void disable(const std::shared_ptr<Port>& swPort);
  void program(const std::shared_ptr<Port>& swPort);
  bool isEnabled();

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
  BcmPortGroup* getPortGroup() const {
    return portGroup_;
  }

  /*
   * Helpers for retreiving the SwitchState node for a given
   * port. There is no great place for this so I am putting it in here
   * for now.
   */
  std::shared_ptr<Port> getSwitchStatePort(
    const std::shared_ptr<SwitchState>& state) const;
  std::shared_ptr<Port> getSwitchStatePortIf(
    const std::shared_ptr<SwitchState>& state) const;

  PortID getPortID() const;
  LaneSpeeds supportedLaneSpeeds() const;

  bool supportsSpeed(cfg::PortSpeed speed);

  /*
   * Setters.
   */
  void registerInPortGroup(BcmPortGroup* portGroup);
  void setPortStatus(bool up);
  void setIngressVlan(const std::shared_ptr<Port>& swPort);
  void setSpeed(const std::shared_ptr<Port>& swPort);

  /*
   * Update this port's statistics.
   */
  void updateStats();

  /**
   * Get the state of the port. If there is an error in finding the port state,
   * then an BcmError() exception is thrown.
   */
  cfg::PortState getState();

  /**
   * Try to remedy this port if this is down.
   */
  void remedy();

  /**
   * Take actions on this port (especially if it is up), so that it will not
   * flap on warm boot.
   */
  void prepareForGracefulExit();

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
                        stats::ExportedHistogramMapImpl::LockableHistogram* hist,
                        const std::vector<opennsl_stat_val_t>& stats);
  std::string statName(folly::StringPiece name) const;

  void disablePause();
  void setAdditionalStats(std::chrono::seconds now);
  void setConfiguredMaxSpeed();
  opennsl_port_if_t getDesiredInterfaceMode(cfg::PortSpeed speed,
                                            PortID id,
                                            std::string name);

  opennsl_pbmp_t getPbmp();

  void setKR4Ability();
  void setFEC(const std::shared_ptr<Port>& swPort);

  BcmSwitch* const hw_{nullptr};
  const opennsl_port_t port_;    // Broadcom physical port number
  // The gport_ is logically a const, but needs to be initialized as a parameter
  // to SDK call.
  opennsl_gport_t gport_;  // Broadcom global port number
  cfg::PortSpeed configuredMaxSpeed_;
  BcmPlatformPort* const platformPort_{nullptr};
  int unit_{-1};

  // The port group this port is a part of
  BcmPortGroup* portGroup_{nullptr};

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

  stats::ExportedStatMapImpl::LockableStat outQueueLen_;
  stats::ExportedHistogramMapImpl::LockableHistogram inPktLengths_;
  stats::ExportedHistogramMapImpl::LockableHistogram outPktLengths_;
};

}} // namespace facebook::fboss
