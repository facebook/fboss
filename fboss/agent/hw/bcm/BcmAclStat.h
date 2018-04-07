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
#include "common/stats/MonotonicCounter.h"
#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

using facebook::stats::MonotonicCounter;

class BcmSwitch;

/**
 *  BcmAclStat is the class to abstract a stat's resource and functions
 */
class BcmAclStat {
 public:
  using BcmAclStatHandle = int;
  BcmAclStat(BcmSwitch* hw, int gid, const std::string& name);
  ~BcmAclStat();
  BcmAclStatHandle getHandle() const {
    return handle_;
  }
  const std::string& getName() const {
    return name_;
  }
  void updateStat();
 private:
  BcmSwitch* hw_;
  std::string name_;
  BcmAclStatHandle handle_;
  MonotonicCounter counter_;
};

}} // facebook::fboss
