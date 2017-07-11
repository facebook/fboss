/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTableStats.h"

namespace facebook { namespace fboss {

bool BcmTableStats::refreshLPMOnlyStats() {
  return false;
}

bool BcmTableStats::refreshHwStatusStats() {
  return false;
}

bool BcmTableStats::refreshLPMStats() {
  return false;
}

void BcmTableStats::publish() const {}
}}
