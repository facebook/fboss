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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/types.h"

namespace facebook::fboss {

class BcmSwitch;

/**
 *  BcmAclStat is the class to abstract a stat's resource and functions
 */
class BcmAclStat {
 public:
  BcmAclStat(
      BcmSwitch* hw,
      int gid,
      const std::vector<cfg::CounterType>& counters);
  BcmAclStat(BcmSwitch* hw, BcmAclStatHandle statHandle)
      : hw_(hw), handle_(statHandle) {}
  ~BcmAclStat();

  BcmAclStatHandle getHandle() const {
    return handle_;
  }

  /**
   * Check whether the acl details of handle in h/w matches the s/w acl and
   * ranges
   */
  static bool isStateSame(
      const BcmSwitch* hw,
      BcmAclStatHandle statHandle,
      cfg::TrafficCounter& counter);

 private:
  BcmSwitch* hw_;
  BcmAclStatHandle handle_;
};

} // namespace facebook::fboss
