/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/EcmpGroupConsolidator.h"

#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> EcmpGroupConsolidator::consolidate(
    const StateDelta& delta) {
  return delta.newState();
}
} // namespace facebook::fboss
