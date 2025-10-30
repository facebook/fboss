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

// Test basic logger creation and destruction
TEST_F(StateDeltaLoggerTest, CreateAndDestroy) {
  {
    StateDeltaLogger logger;
  }

  // Should have created a log file with header
  auto content = readLogFile();
  EXPECT_FALSE(content.empty());
  // The boot header should be written
  EXPECT_NE(content.find("Start of a"), std::string::npos);
}

// Test logging a state delta with oper delta serialization and reconstruction
// Tests BINARY and COMPACT serialization protocols
// SIMPLE_JSON is tested separately due to different handling
TEST_F(StateDeltaLoggerTest, LogStateDeltaAndReconstruct) {
  // Test each serialization protocol (excluding SIMPLE_JSON which needs special
  // handling)
  std::vector<std::pair<std::string, fsdb::OperProtocol>> protocols = {
      {"BINARY", fsdb::OperProtocol::BINARY},
      {"COMPACT", fsdb::OperProtocol::COMPACT},
      {"SIMPLE_JSON", fsdb::OperProtocol::SIMPLE_JSON}};

  for (const auto& [protocolName, protocol] : protocols) {
    SCOPED_TRACE("Testing protocol: " + protocolName);

    // Create new temp file for this protocol
    auto tempFile = std::make_unique<folly::test::TemporaryFile>();
    FLAGS_state_delta_log_file = tempFile->path().string();
    FLAGS_state_delta_log_protocol = protocolName;

    // Create two switch states with different IPv6 routes
    auto state1 = makeStateWithRoute("2001:db8:1::", 64);
    auto state2 = makeStateWithRoute("2001:db8:2::", 64);

    // Create a StateDelta - the oper delta will be automatically computed
    StateDelta delta(state1, state2);

    // Get the original oper delta
    const auto& originalOperDelta = delta.getOperDelta();

    // Create logger and log the delta
    {
      StateDeltaLogger logger;
      logger.logStateDelta(delta, "test_reason_" + protocolName);
    }

    // Read the log file
    std::string logContent;
    ASSERT_TRUE(folly::readFile(tempFile->path().string().c_str(), logContent))
        << "Failed to read log file for protocol: " << protocolName;
    EXPECT_FALSE(logContent.empty());

    // Parse the log file to extract serialized oper delta
    // The log format is JSON with "oper_delta" field
    auto operDeltaPos = logContent.find("\"oper_delta\":\"");
    ASSERT_NE(operDeltaPos, std::string::npos)
        << "Could not find oper_delta in log for protocol: " << protocolName;

    // Extract the serialized oper delta
    auto startPos = operDeltaPos + strlen("\"oper_delta\":\"");
    auto endPos = logContent.find("\"}\n", startPos);
    ASSERT_NE(endPos, std::string::npos);

    auto serializedOperDelta = logContent.substr(startPos, endPos - startPos);
    EXPECT_FALSE(serializedOperDelta.empty())
        << "Empty serialized oper delta for protocol: " << protocolName;

    // Deserialize the oper delta using the same protocol
    fsdb::OperDelta reconstructedOperDelta;
    try {
      using TC = apache::thrift::type_class::structure;
      reconstructedOperDelta = thrift_cow::deserialize<TC, fsdb::OperDelta>(
          protocol, serializedOperDelta);
    } catch (const std::exception& ex) {
      FAIL() << "Failed to deserialize oper delta for protocol: "
             << protocolName << ", error: " << ex.what();
    }

    // Verify the reconstructed oper delta matches the original
    EXPECT_EQ(reconstructedOperDelta, originalOperDelta)
        << "Reconstructed oper delta does not match original for protocol: "
        << protocolName;
  }
}
