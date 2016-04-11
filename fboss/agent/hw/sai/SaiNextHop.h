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
#include <set>

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
   *  
   * @return ID of Next Hop if it's already programmed 
   *         to HW, otherwise SAI_NULL_OBJECT_ID.
   */
  sai_object_id_t GetSaiNextHopId() const;

  /**
   * Checks whether the Next Hop is resolved.  
   *  
   * @return 'true' if Next Hop is resolved otherwise 'false'
   */
  bool isResolved() const {
    return resolved_;
  }

  /**
   * Programs Next Hop to HW in case the the unresolved 
   * Next Hop with "ip" is resolved. Stores
   * SAI Next Hop ID or adds it to Next Hop Group ID(if fwdInfo_ 
   * contains several ECMP next hops) returned from the HW.   
   *  
   * @return none
   */
  void onResolved(InterfaceID intf, const folly::IPAddress &ip);

  /**
   * Programs next hop(s) from the fwdInfo_ in HW and stores
   * SAI Next Hop ID or SAI Next Hop Group ID(if fwdInfo_ 
   * contains several ECMP next hops) returned from the HW.   
   *  
   * @return none
   */
  void Program();

private:
  // no copy or assignment
  SaiNextHop(SaiNextHop const &) = delete;
  SaiNextHop &operator=(SaiNextHop const &) = delete;

  /**
   * Programs single Next Hop on HW.
   *  
   * @return ID of the Next Hop programmed or SAI_NULL_OBJECT_ID in 
   *         case of fail. 
   */
  sai_object_id_t programNh(InterfaceID intf, const folly::IPAddress &ip);
  /**
   * Programs Next Hop group with Next Hops IDs "nhIds" on HW.
   *  
   * @return ID of the Next Hop Group programmed or SAI_NULL_OBJECT_ID in 
   *         case of fail. 
   */
  sai_object_id_t programNhGroup(uint32_t nhCount, sai_object_id_t *nhIds);

  const SaiSwitch *hw_ {nullptr};

  RouteForwardInfo fwdInfo_;
  bool resolved_ {false};
  bool isGroup_ {false};
  std::set<RouteForwardInfo::Nexthop> unresolvedNextHops_;
  std::set<RouteForwardInfo::Nexthop> resolvedNextHops_;
  std::map<sai_object_id_t, RouteForwardInfo::Nexthop> hwNextHops_;
  sai_object_id_t nhGroupId_ {SAI_NULL_OBJECT_ID};

  sai_next_hop_api_t *pSaiNextHopApi_ {nullptr};
  sai_next_hop_group_api_t *pSaiNextHopGroupApi_ {nullptr};
};

}} // facebook::fboss
