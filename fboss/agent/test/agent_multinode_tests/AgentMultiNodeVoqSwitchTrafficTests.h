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

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchNeighborTests.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchTrafficTest
    : public AgentMultiNodeVoqSwitchNeighborTest {
 private:
  std::pair<folly::IPAddressV6, int16_t> kGetRoutePrefixAndPrefixLength()
      const {
    return std::make_pair(folly::IPAddressV6("2001:0db8:85a3::"), 64);
  }

  std::map<std::string, utility::Neighbor>
  configureNeighborsAndRoutesForTrafficLoop(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const;

  void injectTraffic(const utility::Neighbor& neighbor) const;

  std::pair<std::string, std::vector<utility::Neighbor>>
  configureRouteToRemoteRdswWithTwoNhops(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const;

  void pumpRoCETraffic(const PortID& localPort) const;

 protected:
  bool setupTrafficLoop(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const;

 public:
  bool verifyTrafficSpray(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const;

  bool verifyShelAndConditionalEntropy(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const;
};

} // namespace facebook::fboss
