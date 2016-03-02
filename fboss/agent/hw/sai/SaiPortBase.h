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
   * @param pSwitch, pointer to SaiSwitch object
   * @param saiPortId, sai_port_id_t port ID 
   * @param fbossPortId, fboss port ID 
   * @param pPlatform, pointer to SaiPlatformPort object
   */
  SaiPortBase(SaiSwitch *pSwitch, sai_object_id_t saiPortId, PortID fbossPortId, SaiPlatformPort *pPlatformPort);
  virtual ~SaiPortBase();

  /**
   * @brief Initialize port
   * @param warmBoot, switch boot type
   */
  void Init(bool warmBoot);

  /*
  * Getters.
  */
  SaiPlatformPort *GetPlatformPort() const {
    return pPlatformPort_;
  }

  SaiSwitch *GetHwSwitch() const {
    return pHw_;
  }

  sai_object_id_t GetSaiPortId() const {
    return saiPortId_;
  }

  PortID GetFbossPortId() const {
    return fbossPortId_;
  }

  VlanID GetIngressVlan() {
    return pvId_;
  }
  /*
  * Setters.
  */
  void SetPortStatus(bool linkStatus);

  void SetIngressVlan(VlanID vlan);

  /*
  * Update this port's statistics.
  */
  void UpdateStats();

private:
  // Forbidden copy constructor and assignment operator
  SaiPortBase(SaiPortBase const &) = delete;
  SaiPortBase &operator=(SaiPortBase const &) = delete;

  std::string StatName(folly::StringPiece name) const;

  SaiSwitch *const pHw_ {
    nullptr
  };         // Pointer to HW Switch

  SaiPlatformPort *const pPlatformPort_ {
    nullptr
  }; // Pointer to Platform port

  sai_object_id_t saiPortId_ {0}; 
  PortID fbossPortId_ {0};
  VlanID pvId_ {0};
  bool linkStatus_ {true};
  bool initDone_ {false};

  sai_port_api_t *pSaiPortApi_ { nullptr };

private:
  class MonotonicCounter : public stats::MonotonicCounter {
  public:
    // Inherit stats::MonotonicCounter constructors
    using stats::MonotonicCounter::MonotonicCounter;

    // Export SUM and RATE by default
    explicit MonotonicCounter(const std::string &name)
      : stats::MonotonicCounter(name, stats::SUM, stats::RATE) {}
  };

  MonotonicCounter inBytes_ { StatName("in_bytes") };
  MonotonicCounter inMulticastPkts_ { StatName("in_multicast_pkts") };
  MonotonicCounter inBroadcastPkts_ { StatName("in_broadcast_pkts") };

  MonotonicCounter outBytes_ { StatName("out_bytes") };
};

}} // facebook::fboss
