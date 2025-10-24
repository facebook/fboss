/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_multinode_tests/TopologyInfo.h"

namespace facebook::fboss::utility {

void verifyDsfCluster(const std::unique_ptr<TopologyInfo>& topologyInfo);
void verifyDsfAgentDownUp();

} // namespace facebook::fboss::utility
