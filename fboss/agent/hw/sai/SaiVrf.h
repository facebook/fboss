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

extern "C" {
#include "saitypes.h"
#include "sairouter.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;

class SaiVrf {
public:
  /**
   * Constructs the SaiVrf object.
   *
   * This method doesn't make any calls to the SAI to program vrf there.
   * Program() will be called soon after construction, and any
   * actual initialization logic has to be performed there.
   */
  explicit SaiVrf(const SaiSwitch *hw, RouterID fbossVrfId);
  /**
   * Destructs the SaiVrf object.
   *
   * This method removes the next hops from the HW as well.
   */
  virtual ~SaiVrf();
  /**
   * Gets Sai Vrf ID of sai_object_id_t type. 
   * Should be called after Program() call. 
   * Till that moment will always throw an exception; 
   *  
   * @return Sai Next Hop of sai_object_id_t type
   */
  sai_object_id_t GetSaiVrfId() const;
  /**
   * Gets FBOSS Vrf ID of RouterID type. 
   *  
   * @return FBOSS Vrf ID of RouterID type
   */
  RouterID GetFbossVrfId() const {
    return fbossVrfId_;
  }

  /**
   * Programs Vrf in HW and stores SAI Vrf ID returned from the HW.   
   * If one of SAI HW calls fails throws an exception; 
   *  
   * @return none
   */
  void Program();

private:
  // no copy or assignment
  SaiVrf(SaiVrf const &) = delete;
  SaiVrf &operator=(SaiVrf const &) = delete;

  enum {
    SAI_VRF_ATTR_COUNT = 2
  };

  const SaiSwitch *hw_ {nullptr};
  RouterID fbossVrfId_ {0};
  sai_object_id_t saiVrfId_ {SAI_NULL_OBJECT_ID};

  sai_virtual_router_api_t *pSaiVirtualRouterApi_ {nullptr};
};

}} // facebook::fboss

