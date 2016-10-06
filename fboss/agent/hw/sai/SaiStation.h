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

#include <folly/MacAddress.h>

#include "fboss/agent/types.h"

extern "C" {
#include "saitypes.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;

class SaiStation {
public:
  // L2 station can be used to filter based on MAC, VLAN, or Port.
  // We only use it to filter based on the MAC to enable the L3 processing.
  explicit SaiStation(const SaiSwitch *hw);
  ~SaiStation();

  void program(folly::MacAddress mac, sai_object_id_t aclTableId);

private:
  // no copy or assignment
  SaiStation(SaiStation const &) = delete;
  SaiStation &operator=(SaiStation const &) = delete;

  enum : int {
    INVALID = -1,
  };
  const SaiSwitch *hw_;
  int id_ {INVALID};
};

}} // facebook::fboss
