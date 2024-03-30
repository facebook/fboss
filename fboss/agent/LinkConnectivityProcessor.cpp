/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/LinkConnectivityProcessor.h"
#include <memory>
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> LinkConnectivityProcessor::process(
    const std::shared_ptr<SwitchState>& in,
    const std::map<PortID, FabricConnectivityDelta>& connectivityDelta) {
  return std::shared_ptr<SwitchState>();
}
} // namespace facebook::fboss
