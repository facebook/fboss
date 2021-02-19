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
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/MultiNodeTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"

using namespace facebook::fboss;

DEFINE_int32(multiNodeTestPort1, 0, "multinode test port 1");
DEFINE_int32(multiNodeTestPort2, 0, "multinode test port 2");
DEFINE_bool(run_forever, false, "run the test forever");

DECLARE_bool(enable_lacp);

using utility::addAggPort;
namespace {
constexpr int kMaxRetries{10};
constexpr int kBaseVlanId{2000};
} // unnamed namespace

class MultiNodeLacpTest : public MultiNodeTest {
  void SetUp() override {
    FLAGS_enable_lacp = true;
    MultiNodeTest::SetUp();
  }
  cfg::SwitchConfig initialConfig() const override {
    return getConfigWithAggPort();
  }

 protected:
  cfg::SwitchConfig getConfigWithAggPort() const {
    auto config = utility::oneL3IntfNPortConfig(
        platform()->getHwSwitch(),
        {PortID(FLAGS_multiNodeTestPort1), PortID(FLAGS_multiNodeTestPort2)},
        cfg::PortLoopbackMode::NONE,
        true, /* interfaceHasSubnet */
        kBaseVlanId,
        false, /* optimizePortProfile */
        false /* setInterfaceMac */);
    addAggPort(
        kAggId, {FLAGS_multiNodeTestPort1, FLAGS_multiNodeTestPort2}, &config);
    return config;
  }

  // Waits for Aggregate port to be up
  bool waitAndCheckForAggPortUp() {
    auto iter = kMaxRetries;
    while (iter--) {
      const auto& aggPorts = sw()->getState()->getAggregatePorts();
      if (aggPorts && aggPorts->getAggregatePort(kAggId) &&
          aggPorts->getAggregatePort(kAggId)->isUp()) {
        return true;
      }
      // wait for remote side to react to lacp control packets
      // and establish session.
      sleep(5);
    }
    const auto& aggPorts = sw()->getState()->getAggregatePorts();
    if (aggPorts && aggPorts->getAggregatePort(kAggId)) {
      const auto& memberAndStates =
          aggPorts->getAggregatePort(kAggId)->subportAndFwdState();
      for (const auto& memberAndState : memberAndStates) {
        if (memberAndState.second != AggregatePort::Forwarding::ENABLED) {
          XLOG(DBG2) << "LACP failed to converge on subport "
                     << memberAndState.first;
        }
      }
    }
    return false;
  }

  // Verify that LACP converged on all member ports
  void verifyLacpState() {
    const auto& aggPort =
        sw()->getState()->getAggregatePorts()->getAggregatePort(kAggId);
    ASSERT_EQ(aggPort->forwardingSubportCount(), aggPort->subportsCount());
    for (const auto& memberAndState : aggPort->subportAndFwdState()) {
      // Verify that member port is enabled
      ASSERT_EQ(memberAndState.second, AggregatePort::Forwarding::ENABLED);
      // Verify that partner has synced
      ASSERT_EQ(
          aggPort->getPartnerState(memberAndState.first).state &
              LacpState::IN_SYNC,
          LacpState::IN_SYNC);
    }
  }
  const facebook::fboss::AggregatePortID kAggId{500};
};

TEST_F(MultiNodeLacpTest, Bringup) {
  // Wait for AggPort
  ASSERT_TRUE(waitAndCheckForAggPortUp());

  // verify lacp state information
  verifyLacpState();

  // Do not tear down setup if run_forver flag is true. This is used on
  // remote side of device under test to keep state running till DUT
  // completes tests. Test will be terminated by the starting script
  // by sending a SIGTERM.
  if (FLAGS_run_forever) {
    XLOG(DBG2) << "MultiNodeLacpTest run forever...";
    while (true) {
      sleep(1);
      XLOG_EVERY_MS(DBG2, 5000) << "MultiNodeLacpTest running forever";
    }
  }
}
