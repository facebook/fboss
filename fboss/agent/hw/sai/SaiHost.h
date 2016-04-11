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

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

#include "fboss/agent/types.h"

extern "C" {
#include "saitypes.h"
#include "saineighbor.h"
#include "saiswitch.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;

class SaiHost {
public:
  /**
   * Constructs the SaiHost object.
   *
   * This method doesn't make any calls to the SAI to program host there.
   * Program() will be called soon after construction, and any
   * actual initialization logic will be performed there.
   */
  SaiHost(const SaiSwitch *hw,
          InterfaceID intf,
          const folly::IPAddress &ip,
          const folly::MacAddress &mac);
  /**
   * Destructs the SaiHost object.
   *
   * This method removes the host from the HW as well.
   */
  virtual ~SaiHost();

  /**
   * Gets Sai Host action. 
   *  
   * @return Host action of sai_packet_action_t type.
   */
  sai_packet_action_t getSaiAction() const {
      return action_;
  }

  /**
   * Programs host in HW and stores sai_neighbor_entry_t instance
   * which holds SAI host data needed. 
   *  
   * If one of SAI HW calls fails it throws an exception; 
   *  
   * @return none
   */
  void Program(sai_packet_action_t action, const folly::MacAddress &mac);

private:
  // no copy or assignment
  SaiHost(SaiHost const &) = delete;
  SaiHost &operator=(SaiHost const &) = delete;
     
  const SaiSwitch *hw_;
  const InterfaceID intf_;
  const folly::IPAddress ip_;
  folly::MacAddress mac_;
  sai_packet_action_t action_ {SAI_PACKET_ACTION_DROP};
  bool added_ {false}; // if added to the HW host table or not

  sai_neighbor_entry_t neighborEntry_;
  sai_neighbor_api_t *pSaiNeighborApi_ { nullptr };
};

}} // facebook::fboss
