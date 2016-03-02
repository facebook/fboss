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

#include <map>

#include "fboss/agent/types.h"

extern "C" {
#include "saitypes.h"
#include "sainexthop.h"
#include "sainexthopgroup.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;
class RouteForwardInfo;

class SaiNextHop {
public:
  /**
   * Constructs the SaiNextHop object.
   *
   * This method doesn't make any calls to the SAI to program next hop there.
   * Program() will be called soon after construction, and any
   * actual initialization logic is be performed there.
   */
  explicit SaiNextHop(const SaiSwitch *hw, const RouteForwardInfo &fwdInfo);
  /**
   * Destructs the SaiNextHop object.
   *
   * This method removes the next hops from the HW as well.
   */
  virtual ~SaiNextHop();

  /**
   * Gets Sai Next Hop ID or Sai Next Hop Group ID 
   * of sai_object_id_t type. 
   * Should be called after Program() call. 
   * Till that moment will always throw an exception; 
   *  
   * @return Sai Next Hop of sai_object_id_t type
   */
  sai_object_id_t GetSaiNextHopId() const;

  /**
   * Programs next hop(s) from the fwdInfo_ in HW and stores
   * SAI Next Hop ID or SAI Next Hop Group ID(if fwdInfo_ 
   * contains several ECMP next hops) returned from the HW.   
   * If one of SAI HW calls fails it throws an exception; 
   *  
   * @return none
   */
  void Program();

private:
  // no copy or assignment
  SaiNextHop(SaiNextHop const &) = delete;
  SaiNextHop &operator=(SaiNextHop const &) = delete;

  enum {
    SAI_NH_GROUP_ATTR_COUNT = 2,
    SAI_NH_ATTR_COUNT = 3
  };

  const SaiSwitch *hw_ {nullptr};

  RouteForwardInfo fwdInfo_;
  std::map<sai_object_id_t, RouteForwardInfo::Nexthop &> nextHops_;
  sai_object_id_t nhGroupId_ {SAI_NULL_OBJECT_ID};

  sai_next_hop_api_t *pSaiNextHopApi_ {nullptr};
  sai_next_hop_group_api_t *pSaiNextHopGroupApi_ {nullptr};
};

}} // facebook::fboss
