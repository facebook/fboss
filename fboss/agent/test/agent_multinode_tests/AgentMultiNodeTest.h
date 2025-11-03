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

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/agent_multinode_tests/TopologyInfo.h"

namespace facebook::fboss {

class AgentMultiNodeTest : public AgentHwTest {
  void SetUp() override;

  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) const override;

  std::vector<ProductionFeature> getProductionFeaturesVerified() const override;

 protected:
  void setCmdLineFlagOverrides() const override;

  void verifyCluster() const;
  void runTestWithVerifyCluster(
      const std::function<bool(const std::unique_ptr<utility::TopologyInfo>&)>&
          testFn) const;

  std::unique_ptr<utility::TopologyInfo> topologyInfo_;
};

} // namespace facebook::fboss
