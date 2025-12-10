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

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchNeighborTest : public AgentMultiNodeTest {
 protected:
  std::vector<utility::Neighbor> computeNeighborsForRdsw(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo,
      const std::string& rdsw,
      const int& numNeighbors) const;

 public:
  bool verifyNeighborAddRemove(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo);
};

} // namespace facebook::fboss
