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

#include <boost/container/flat_map.hpp>

#include "fboss/agent/state/RouteForwardInfo.h"

extern "C" {
#include "saitypes.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;
class SaiNextHop;

class SaiNextHopTable {
public:
  explicit SaiNextHopTable(const SaiSwitch *hw);
  virtual ~SaiNextHopTable();

  /**
   * Given fwdInfo gets Sai Next Hop ID of sai_object_id_t type. 
   * Throws an exception if there is no such Next Hop found.
   *  
   * @return Sai Vrf ID of sai_object_id_t type
   */
  sai_object_id_t GetSaiNextHopId(const RouteForwardInfo &fwdInfo) const;

  /*
   * The following functions will modify the object. They rely on the global
   * HW update lock in SaiSwitch::lock_ for the protection.
   *
   * SaiNextHopTable maintains a reference counter for each SaiNextHop
   * entry allocated.
   */

  /**
   * Allocates a new SaiNextHop if no such one exists. For the existing
   * entry, incRefOrCreateSaiNextHop() will increase the reference counter by 1. 
   *  
   * @return The SaiNextHop pointer just created or found.
   */
  SaiNextHop* IncRefOrCreateSaiNextHop(const RouteForwardInfo &fwdInfo);

  /**
   * Decreases an existing SaiNextHop entry's reference counter by 1.
   * Only when the reference counter is 0, the SaiNextHop entry
   * is deleted.
   *
   * @return nullptr, if the SaiNextHop entry is deleted
   * @retrun the SaiNextHop object that has reference counter
   *         decreased by 1, but the object is still valid as it is
   *         still referred in somewhere else
   */
  SaiNextHop *DerefSaiNextHop(const RouteForwardInfo &fwdInfo) noexcept;

private:
  typedef std::pair<std::unique_ptr<SaiNextHop>, uint32_t> NextHopMapNode; 
  typedef boost::container::flat_map<RouteForwardNexthops, NextHopMapNode> NextHopMap;

  const SaiSwitch *hw_;
  NextHopMap nextHops_;
};

}} // facebook::fboss
