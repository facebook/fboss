/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StateDeltaLogger.h"

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

using namespace facebook::fboss;

class StateDeltaLoggerTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Create a temporary file for logging
    tempLogFile_ = std::make_unique<folly::test::TemporaryFile>();

    // Configure logging
    FLAGS_enable_state_delta_logging = true;
    FLAGS_state_delta_log_file = tempLogFile_->path().string();
    FLAGS_state_delta_log_protocol = "COMPACT";
    FLAGS_state_delta_log_timeout_ms = 5000;
  }

  void TearDown() override {
    FLAGS_enable_state_delta_logging = false;
  }

  std::string readLogFile() {
    std::string content;
    if (!folly::readFile(tempLogFile_->path().string().c_str(), content)) {
      return "";
    }
    return content;
  }

  std::unique_ptr<folly::test::TemporaryFile> tempLogFile_;

  // Helper function to create a state with an IPv6 route
  std::shared_ptr<SwitchState> makeStateWithRoute(
      const std::string& prefix,
      uint8_t mask) {
    auto state = std::make_shared<SwitchState>();

    // Create IPv6 route
    RoutePrefixV6 routePrefix{folly::IPAddressV6(prefix), mask};
    auto route = std::make_shared<RouteV6>(RouteV6::makeThrift(routePrefix));

    // Create next hop entry with IPv6 address
    std::vector<ResolvedNextHop> nextHops;
    nextHops.emplace_back(
        folly::IPAddress("2401:db00::1"), InterfaceID(1), 1 /* weight */);
    RouteNextHopSet nhopSet(nextHops.begin(), nextHops.end());
    route->setResolved(RouteNextHopEntry(nhopSet, AdminDistance::EBGP));
    route->publish();

    // Add route to FIB
    auto fibContainer =
        std::make_shared<ForwardingInformationBaseContainer>(RouterID(0));
    ForwardingInformationBaseV6 fibV6;
    fibV6.addNode(route);
    fibContainer->setFib(fibV6.clone());

    auto fibMap = std::make_shared<MultiSwitchForwardingInformationBaseMap>();
    fibMap->addNode(fibContainer, HwSwitchMatcher());
    state->resetForwardingInformationBases(fibMap);

    return state;
  }
};
