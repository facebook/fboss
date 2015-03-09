/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/CounterCache.h"

#include "common/stats/ServiceData.h"
#include "fboss/agent/SwSwitch.h"

namespace facebook { namespace fboss {

void CounterCache::update() {
  if (sw_) {
    sw_->publishStats();
  }
  prev_.swap(current_);
  current_.clear();
  fbData->getCounters(current_);
}

}} // facebook::fboss
