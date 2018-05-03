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
#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

class BcmSwitch;

/**
 *  BcmAclStat is the class to abstract a stat's resource and functions
 */
class BcmAclStat {
 public:
  using BcmAclStatHandle = int;
  BcmAclStat(BcmSwitch* hw, int gid);
  ~BcmAclStat();

  BcmAclStatHandle getHandle() const {
    return handle_;
  }
 private:
  BcmSwitch* hw_;
  BcmAclStatHandle handle_;
};

}} // facebook::fboss
