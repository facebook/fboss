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

#include <memory>

extern "C" {
#include <bcm/types.h>
}

#include "fboss/agent/hw/bcm/BcmTrunkStats.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class BcmSwitchIf;

class BcmTrunk {
 public:
  enum : bcm_trunk_t {
    INVALID = -1,
  };

  explicit BcmTrunk(const BcmSwitchIf* hw);
  ~BcmTrunk();

  bcm_trunk_t id() const {
    return bcmTrunkID_;
  }

  void init(const std::shared_ptr<AggregatePort>& aggPort);
  void program(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort,
      std::vector<PortID>& addedPorts,
      std::vector<PortID>& removedPorts);

  static void shrinkTrunkGroupHwNotLocked(
      int unit,
      bcm_trunk_t trunk,
      bcm_port_t toDisable);
  static int getEnabledMemberPortsCountHwNotLocked(
      int unit,
      bcm_trunk_t trunk,
      bcm_port_t port);
  static std::optional<int> findTrunk(int, bcm_module_t, bcm_port_t);

  static bcm_gport_t asGPort(bcm_trunk_t trunk);
  static bool isValidTrunkPort(bcm_gport_t gPort);

  BcmTrunkStats& stats();

 private:
  static int rtag7();
  void suppressTrunkInternalFlood(
      const std::shared_ptr<AggregatePort>& aggPort);
  void programForwardingState(
      LegacyAggregatePortFields::SubportToForwardingState oldState,
      LegacyAggregatePortFields::SubportToForwardingState newState,
      std::vector<PortID>& addedPorts,
      std::vector<PortID>& removedPorts);
  void modifyMemberPort(bool added, PortID memberPort);

  // Forbidden copy constructor and assignment operator
  BcmTrunk(const BcmTrunk&) = delete;
  BcmTrunk& operator=(const BcmTrunk&) = delete;

  bcm_trunk_t bcmTrunkID_{INVALID};
  const BcmSwitchIf* const hw_{nullptr};
  BcmTrunkStats trunkStats_;
};

} // namespace facebook::fboss
