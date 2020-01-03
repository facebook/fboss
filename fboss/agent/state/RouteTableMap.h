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

#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;
class RouteTable;
typedef NodeMapTraits<RouterID, RouteTable> RouteTableMapTraits;

/*
 * A container for the set of VLANs.
 */
class RouteTableMap : public NodeMapT<RouteTableMap, RouteTableMapTraits> {
 public:
  RouteTableMap();
  ~RouteTableMap() override;

  /*
   * Get the specified RouteTable.
   *
   * Throws an FbossError if the ROUTETABLE does not exist.
   */
  const std::shared_ptr<RouteTable>& getRouteTable(RouterID id) const {
    return getNode(id);
  }

  /*
   * Get the specified RouteTable.
   *
   * Returns null if the RouteTable does not exist.
   */
  std::shared_ptr<RouteTable> getRouteTableIf(RouterID id) const {
    return getNodeIf(id);
  }

  RouteTableMap* modify(std::shared_ptr<SwitchState>* state);

  /**
   * Get the v4 and v6 route count
   */
  void getRouteCount(uint64_t* v4Count, uint64_t* v6Count);

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */

  void addRouteTable(const std::shared_ptr<RouteTable>& rt);

  void updateRouteTable(const std::shared_ptr<RouteTable>& rt);

  void removeRouteTable(const std::shared_ptr<RouteTable>& rt);

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
