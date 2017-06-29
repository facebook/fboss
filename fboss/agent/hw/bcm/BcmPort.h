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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/gen-cpp2/hardware_stats_types.h"

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
  bool isEnabled();
  cfg::PortSpeed getSpeed();

  /*
   * Setters.
   */
  void registerInPortGroup(BcmPortGroup* portGroup);
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
   * Take actions on this port (especially if it is up), so that it will not
   * flap on warm boot.
   */
  void prepareForGracefulExit();

  /**
   * return true iff the port has Forward Error Correction (FEC)
   * enabled
   */
  bool isFECEnabled();

  void updateName(const std::string& newName);

  /*
   * Take the appropriate actions for reacting to the port's state changing.
   * e.g. optionally remediate, turn on/off LEDs
   */
  void linkStatusChanged(const std::shared_ptr<Port>& port);

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

  MonotonicCounter* getPortCounterIf(const std::string& statName);
  bool shouldReportStats() const;
  void reinitPortStats();
  void reinitPortStat(const std::string& newName);
  void updateStat(std::chrono::seconds now,
                  const std::string& statName,
                  opennsl_stat_val_t type,
                  int64_t* portStatVal);
  void updatePktLenHist(std::chrono::seconds now,
                        stats::ExportedHistogramMapImpl::LockableHistogram* hist,
                        const std::vector<opennsl_stat_val_t>& stats);
  // Set stats that are either FB specific, not available in
  // open source opennsl release.
  void setAdditionalStats(std::chrono::seconds now, HwPortStats* curPortStats);
  std::string statName(folly::StringPiece name) const;

  void disablePause();
  void setConfiguredMaxSpeed();
  opennsl_port_if_t getDesiredInterfaceMode(cfg::PortSpeed speed,
                                            PortID id,
                                            const std::string& name);
  TransmitterTechnology getTransmitterTechnology(const std::string& name);

  opennsl_pbmp_t getPbmp();

  void setKR4Ability();
  void setFEC(const std::shared_ptr<Port>& swPort);
  bool isMmuLossy() const;

  static constexpr auto kOutCongestionDiscards = "out_congestion_discards";

  BcmSwitch* const hw_{nullptr};
  const opennsl_port_t port_;    // Broadcom physical port number
  // The gport_ is logically a const, but needs to be initialized as a parameter
  // to SDK call.
  opennsl_gport_t gport_;  // Broadcom global port number
  cfg::PortSpeed configuredMaxSpeed_;
  BcmPlatformPort* const platformPort_{nullptr};
  int unit_{-1};
  std::string portName_{""};
  TransmitterTechnology transmitterTechnology_{TransmitterTechnology::UNKNOWN};

  // The port group this port is a part of
  BcmPortGroup* portGroup_{nullptr};

  std::map<std::string, MonotonicCounter> portCounters_;

  stats::ExportedStatMapImpl::LockableStat outQueueLen_;
  stats::ExportedHistogramMapImpl::LockableHistogram inPktLengths_;
  stats::ExportedHistogramMapImpl::LockableHistogram outPktLengths_;
  HwPortStats portStats_;
};

}} // namespace facebook::fboss
