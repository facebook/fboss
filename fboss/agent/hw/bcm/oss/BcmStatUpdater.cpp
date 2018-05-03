/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"

namespace facebook { namespace fboss {

using facebook::stats::MonotonicCounter;

void BcmStatUpdater::updateAclStat(
  int /*unit*/,
  BcmAclStatHandle /*handle*/,
  std::chrono::seconds /*now*/,
  MonotonicCounter* /*counter*/) {}

}} // facebook::fboss
