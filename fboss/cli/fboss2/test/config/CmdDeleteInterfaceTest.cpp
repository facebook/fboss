/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/delete/interface/CmdDeleteInterface.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

// ---------------------------------------------------------------------------
// Fixture — one port has loopbackMode=PHY and expectedLLDPValues (PORT tag),
//           the other has neither set
// ---------------------------------------------------------------------------

class CmdDeleteInterfaceTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteInterfaceTestFixture()
      : CmdConfigTestBase(
            "fboss_delete_interface_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000,
        "loopbackMode": 1,
        "lookupClasses": [10, 11],
        "expectedLLDPValues": {
          "2": "ge-0/0/0"
        }
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000
      }
    ],
    "interfaces": []
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "delete interface";
};

// ---------------------------------------------------------------------------
// queryClient: resets loopbackMode to NONE
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientResetsLoopbackMode) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 loopback-mode");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs = InterfaceDeleteConfig({"eth1/1/1", "loopback-mode"});

  auto result = cmd.queryClient(localhost(), deleteAttrs);

  EXPECT_THAT(result, HasSubstr("loopback-mode"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  auto& ports = *ConfigSession::getInstance().getAgentConfig().sw()->ports();
  EXPECT_EQ(*ports[0].loopbackMode(), cfg::PortLoopbackMode::NONE);
}

// ---------------------------------------------------------------------------
// queryClient: idempotent when port has no loopbackMode set
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientIdempotentNoLoopbackMode) {
  // eth1/2/1 has no loopbackMode in the initial config
  setupTestableConfigSession(cmdPrefix_, "eth1/2/1 loopback-mode");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs = InterfaceDeleteConfig({"eth1/2/1", "loopback-mode"});

  EXPECT_NO_THROW({
    auto result = cmd.queryClient(localhost(), deleteAttrs);
    EXPECT_THAT(result, HasSubstr("loopback-mode"));
  });
}

// ---------------------------------------------------------------------------
// queryClient: resets loopbackMode across multiple ports
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientMultiplePortsLoopbackMode) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 eth1/2/1 loopback-mode");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs =
      InterfaceDeleteConfig({"eth1/1/1", "eth1/2/1", "loopback-mode"});

  auto result = cmd.queryClient(localhost(), deleteAttrs);

  EXPECT_THAT(result, HasSubstr("loopback-mode"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));

  auto& ports = *ConfigSession::getInstance().getAgentConfig().sw()->ports();
  for (const auto& port : ports) {
    EXPECT_EQ(*port.loopbackMode(), cfg::PortLoopbackMode::NONE);
  }
}

// ---------------------------------------------------------------------------
// queryClient: removes PORT entry from expectedLLDPValues
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientRemovesLldpExpectedValue) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 lldp-expected-value");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs = InterfaceDeleteConfig({"eth1/1/1", "lldp-expected-value"});

  auto result = cmd.queryClient(localhost(), deleteAttrs);

  EXPECT_THAT(result, HasSubstr("lldp-expected-value"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  auto& ports = *ConfigSession::getInstance().getAgentConfig().sw()->ports();
  const auto& lldpValues = *ports[0].expectedLLDPValues();
  EXPECT_EQ(lldpValues.count(cfg::LLDPTag::PORT), 0);
}

// ---------------------------------------------------------------------------
// queryClient: idempotent when port has no expectedLLDPValues
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientIdempotentNoLldpValues) {
  // eth1/2/1 has no expectedLLDPValues in the initial config
  setupTestableConfigSession(cmdPrefix_, "eth1/2/1 lldp-expected-value");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs = InterfaceDeleteConfig({"eth1/2/1", "lldp-expected-value"});

  EXPECT_NO_THROW({
    auto result = cmd.queryClient(localhost(), deleteAttrs);
    EXPECT_THAT(result, HasSubstr("lldp-expected-value"));
  });
}

// ---------------------------------------------------------------------------
// queryClient: removes PORT entry from multiple ports
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientMultiplePortsLldpValue) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 eth1/2/1 lldp-expected-value");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs =
      InterfaceDeleteConfig({"eth1/1/1", "eth1/2/1", "lldp-expected-value"});

  auto result = cmd.queryClient(localhost(), deleteAttrs);

  EXPECT_THAT(result, HasSubstr("lldp-expected-value"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));

  auto& ports = *ConfigSession::getInstance().getAgentConfig().sw()->ports();
  for (const auto& port : ports) {
    EXPECT_EQ(port.expectedLLDPValues()->count(cfg::LLDPTag::PORT), 0);
  }
}

// ---------------------------------------------------------------------------
// queryClient: resets both loopback-mode and lldp-expected-value in one call
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientMultipleAttrs) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 loopback-mode lldp-expected-value");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs = InterfaceDeleteConfig(
      {"eth1/1/1", "loopback-mode", "lldp-expected-value"});

  auto result = cmd.queryClient(localhost(), deleteAttrs);

  EXPECT_THAT(result, HasSubstr("loopback-mode"));
  EXPECT_THAT(result, HasSubstr("lldp-expected-value"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  auto& ports = *ConfigSession::getInstance().getAgentConfig().sw()->ports();
  EXPECT_EQ(*ports[0].loopbackMode(), cfg::PortLoopbackMode::NONE);
  EXPECT_EQ(ports[0].expectedLLDPValues()->count(cfg::LLDPTag::PORT), 0);
}

// ---------------------------------------------------------------------------
// InterfaceDeleteConfig constructor: rejects unknown attribute
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, unknownAttrRejected) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 unknown-attr");
  EXPECT_THROW(
      InterfaceDeleteConfig({"eth1/1/1", "unknown-attr"}),
      std::invalid_argument);
}

// ---------------------------------------------------------------------------
// printOutput test
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, printOutput) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 loopback-mode");
  auto cmd = CmdDeleteInterface();
  testing::internal::CaptureStdout();
  cmd.printOutput("test output");
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, HasSubstr("test output"));
}

// ---------------------------------------------------------------------------
// queryClient: clears lookupClasses back to empty
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientClearsLookupClasses) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 lookup-class");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs = InterfaceDeleteConfig({"eth1/1/1", "lookup-class"});

  auto result = cmd.queryClient(localhost(), deleteAttrs);

  EXPECT_THAT(result, HasSubstr("lookup-class"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  auto& ports = *ConfigSession::getInstance().getAgentConfig().sw()->ports();
  EXPECT_TRUE(ports[0].lookupClasses()->empty());
}

// ---------------------------------------------------------------------------
// queryClient: idempotent when port has no lookupClasses set
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientIdempotentNoLookupClasses) {
  // eth1/2/1 has no lookupClasses in the initial config
  setupTestableConfigSession(cmdPrefix_, "eth1/2/1 lookup-class");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs = InterfaceDeleteConfig({"eth1/2/1", "lookup-class"});

  EXPECT_NO_THROW({
    auto result = cmd.queryClient(localhost(), deleteAttrs);
    EXPECT_THAT(result, HasSubstr("lookup-class"));
  });
}

// ---------------------------------------------------------------------------
// queryClient: clears lookupClasses across multiple ports
// ---------------------------------------------------------------------------

TEST_F(CmdDeleteInterfaceTestFixture, queryClientMultiplePortsLookupClasses) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 eth1/2/1 lookup-class");
  auto cmd = CmdDeleteInterface();
  auto deleteAttrs =
      InterfaceDeleteConfig({"eth1/1/1", "eth1/2/1", "lookup-class"});

  auto result = cmd.queryClient(localhost(), deleteAttrs);

  EXPECT_THAT(result, HasSubstr("lookup-class"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));

  auto& ports = *ConfigSession::getInstance().getAgentConfig().sw()->ports();
  for (const auto& port : ports) {
    EXPECT_TRUE(port.lookupClasses()->empty());
  }
}

} // namespace facebook::fboss
