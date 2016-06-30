/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#pragma once

#include "fboss/agent/types.h"
#include "common/stats/MonotonicCounter.h"

extern "C" {
#include "saitypes.h"
#include "saiport.h"
}

namespace facebook { namespace fboss {

using namespace facebook;

class SaiPlatformPort;
class SaiSwitch;

class SaiPortBase {
public:
  /**
   * @brief Constructor. All initialization steps should be done through init
   * @param hw, pointer to SaiSwitch object
   * @param saiPortId, sai_port_id_t port ID 
   * @param fbossPortId, fboss port ID 
   * @param platformPort, pointer to SaiPlatformPort object
   */
  SaiPortBase(SaiSwitch *hw, sai_object_id_t saiPortId, PortID fbossPortId, SaiPlatformPort *platformPort);
  virtual ~SaiPortBase();

  /**
   * @brief initialize port
   * @param warmBoot, switch boot type
   */
  void init(bool warmBoot);

  /*
  * Getters.
  */
  SaiPlatformPort *getPlatformPort() const {
    return platformPort_;
  }

  SaiSwitch *getHwSwitch() const {
    return hw_;
  }

  sai_object_id_t getSaiPortId() const {
    return saiPortId_;
  }

  PortID getFbossPortId() const {
    return fbossPortId_;
  }

  VlanID getIngressVlan() {
    return pvId_;
  }

  bool isEnabled() {
    return adminMode_;
  }
  /*
  * Setters.
  */
  void enable();
  void disable();
  void setPortStatus(bool linkStatus);

  void setIngressVlan(VlanID vlan);

  /*
  * Update this port's statistics.
  */
  void updateStats();

private:
  // Forbidden copy constructor and assignment operator
  SaiPortBase(SaiPortBase const &) = delete;
  SaiPortBase &operator=(SaiPortBase const &) = delete;

  std::string statName(folly::StringPiece name) const;

  SaiSwitch *const hw_ { nullptr };   // Pointer to HW Switch
  SaiPlatformPort *const platformPort_ { nullptr }; // Pointer to Platform port

  sai_object_id_t saiPortId_ {0}; 
  PortID fbossPortId_ {0};
  VlanID pvId_ {0};
  bool adminMode_ {false};
  bool linkStatus_ {true};
  bool initDone_ {false};

  sai_port_api_t *saiPortApi_ { nullptr };

private:
  class MonotonicCounter : public stats::MonotonicCounter {
  public:
    // Inherit stats::MonotonicCounter constructors
    using stats::MonotonicCounter::MonotonicCounter;

    // Export SUM and RATE by default
    explicit MonotonicCounter(const std::string &name)
      : stats::MonotonicCounter(name, stats::SUM, stats::RATE) {}
  };

  MonotonicCounter inBytes_ { statName("in_bytes") };
  MonotonicCounter inMulticastPkts_ { statName("in_multicast_pkts") };
  MonotonicCounter inBroadcastPkts_ { statName("in_broadcast_pkts") };

  MonotonicCounter outBytes_ { statName("out_bytes") };
};

}} // facebook::fboss
