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

#include <boost/container/flat_map.hpp>
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook { namespace fboss {

class NeighborUpdaterImpl;
class SwSwitch;
class SwitchState;
class StateDelta;

/**
 * This class handles asynchronous updates to neighbor tables that are not
 * in response to handling a specific packet. For now this is only used to
 * remove pending entries from the tables after they timeout.
 *
 * This will be used to expire neighbor entries as well once that is
 * implemented.
 */
class NeighborUpdater {
 public:
  explicit NeighborUpdater(SwSwitch* sw);
  ~NeighborUpdater();

  void stateChanged(const StateDelta& delta);
 private:
  void vlanAdded(const SwitchState* state, const Vlan* vlan);
  void vlanDeleted(const Vlan* vlan);


  /**
   * updaters_ should only ever be accessed from the state update thread,
   * so we don't need to lock accesses.
   */
  boost::container::flat_map<VlanID, NeighborUpdaterImpl*> updaters_;
  SwSwitch* sw_{nullptr};
};

}} // facebook::fboss
