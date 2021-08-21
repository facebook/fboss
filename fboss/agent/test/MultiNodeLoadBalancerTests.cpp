/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/init/Init.h>
#include <gtest/gtest.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/LacpMachines.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/MultiNodeTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"

#include "common/process/Process.h"
using namespace facebook::fboss;

DECLARE_bool(enable_lacp);

namespace {
constexpr int kMaxRetries{60};
} // unnamed namespace

class MultiNodeLoadBalancerTest : public MultiNodeTest {
 public:
  void SetUp() override {
    MultiNodeTest::SetUp();
    if (isDUT()) {
      // TOOD - call verify reachability once configs are
      // fixed to generate the right intf IPs
      // verifyReachability();
    }
  }

 private:
  void verifyReachability() const {
    // TODO - figure these out from config
    auto dstIps = {"1::1", "2::1", "1.1.1.1", "2.1.1.1"};
    for (auto dstIp : dstIps) {
      std::string pingCmd = "ping -c 5 ";
      std::string resultStr;
      std::string errStr;
      EXPECT_TRUE(facebook::process::Process::execShellCmd(
          pingCmd + dstIp, &resultStr, &errStr));
    }
  }
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        platform()->getHwSwitch(),
        {PortID(FLAGS_multiNodeTestPort1), PortID(FLAGS_multiNodeTestPort2)},
        cfg::PortLoopbackMode::NONE,
        true /*interfaceHasSubnet*/,
        false /*setInterfaceMac*/,
        2000);
  }
};

TEST_F(MultiNodeLoadBalancerTest, verifyFullHashLoadBalance) {
  // TODO
}
