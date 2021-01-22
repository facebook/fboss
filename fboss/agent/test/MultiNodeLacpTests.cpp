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

using namespace facebook::fboss;

DEFINE_int32(multiNodeTestPort1, 0, "multinode test port 1");
DEFINE_int32(multiNodeTestPort2, 0, "multinode test port 2");

DECLARE_bool(enable_lacp);

class MultiNodeLacpTest : public MultiNodeTest {
  void SetUp() override {
    FLAGS_enable_lacp = true;
    MultiNodeTest::SetUp();
  }
  cfg::SwitchConfig initialConfig() const override {
    auto config = utility::oneL3IntfConfig(
        platform()->getHwSwitch(),
        PortID(FLAGS_multiNodeTestPort1),
        cfg::PortLoopbackMode::NONE,
        2000);

    // This config has an aggregate port comprised of two physical ports
    config.aggregatePorts_ref()->resize(1);
    config.aggregatePorts_ref()[0].key_ref() = 1;
    *config.aggregatePorts_ref()[0].name_ref() = "port-channel";
    *config.aggregatePorts_ref()[0].description_ref() = "two member bundle";
    config.aggregatePorts_ref()[0].memberPorts_ref()->resize(2);
    config.aggregatePorts_ref()[0].memberPorts_ref()[0].memberPortID_ref() =
        FLAGS_multiNodeTestPort1;
    config.aggregatePorts_ref()[0].memberPorts_ref()[1].memberPortID_ref() =
        FLAGS_multiNodeTestPort2;

    return config;
  }
};

TEST_F(MultiNodeLacpTest, multiNodeLacpBringup) {
  // TODO - Add verification for protocol convergence with remote node
}
