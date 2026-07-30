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

#include <fmt/format.h>

#include <iostream>
#include <map>
#include <string>

#include "fboss/cli/fboss2/commands/config/srv6/my_sid/CmdConfigSrv6MySid.h"
#include "fboss/cli/fboss2/commands/config/srv6/my_sid/add/CmdConfigSrv6MySidAdd.h"
#include "fboss/cli/fboss2/commands/config/srv6/my_sid/delete/CmdConfigSrv6MySidDelete.h"
#include "fboss/cli/fboss2/commands/config/srv6/utils/Srv6MySidCliUtils.h"
#include "fboss/cli/fboss2/commands/delete/srv6/my_sid/CmdDeleteSrv6MySid.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

namespace {
const std::string kLocatorPrefix = "fdad:ffff::/32";
const std::string kOtherLocatorPrefix = "fd00:abcd::/32";

const std::map<std::string, std::string> kSrv6MySidTestDescriptions = {
    {"locatorPrefixArg_valid",
     "Valid IPv6 /32 locator prefix is accepted by LocatorPrefixArg"},
    {"locatorPrefixArg_notIPv6", "Rejects non-IPv6 prefix (192.168.1.0/24)"},
    {"locatorPrefixArg_invalid", "Rejects malformed prefix string"},
    {"locatorPrefixArg_unsupportedLength",
     "Rejects IPv6 locator prefixes other than /32"},
    {"locatorPrefixArg_missing", "Rejects missing locator prefix argument"},
    {"mySidInit_createsConfig",
     "config srv6 my-sid <prefix> creates empty mySidConfig in session"},
    {"mySidInit_rejectsSecondInit",
     "Second config srv6 my-sid init for same prefix is rejected"},
    {"mySidAdd_validAdjacency",
     "Parses add entry adjacency args (fn, is-v6, port-name)"},
    {"mySidAdd_validNode", "Parses add entry node args (fn, node-address)"},
    {"mySidAdd_validDecap", "Parses add entry decap args (fn only)"},
    {"mySidAdd_noLocatorThrows",
     "add entry before my-sid init fails with runtime_error"},
    {"mySidAdd_invalidEntry",
     "Rejects function values 0, 99999, and non-numeric abc"},
    {"mySidAdd_unknownType", "Rejects unknown entry type 'binding'"},
    {"mySidAdd_invalidIsV6",
     "Rejects non-boolean is-v6 value on adjacency entry"},
    {"mySidAdd_portNameOnNode", "Rejects port-name on node-type entry"},
    {"mySidAdd_nodeAddressOnAdjacency",
     "Rejects node-address on adjacency-type entry"},
    {"mySidAdd_insertsEntry",
     "add entry adjacency stages entry in session mySidConfig"},
    {"mySidAdd_prefixMismatch",
     "add entry with wrong locator prefix is rejected"},
    {"mySidAdd_upserts",
     "Re-adding same function ID overwrites prior entry with warning"},
    {"mySidDeleteEntry_removesEntry",
     "config srv6 my-sid <prefix> delete entry <fn> removes entry"},
    {"mySidDeleteEntry_noOpWhenMissing",
     "delete entry for missing function ID returns informational message"},
    {"mySidDeleteEntry_noOpWhenNoConfig",
     "delete entry with no mySidConfig returns informational message"},
    {"deleteMySid_removesEntireConfig",
     "delete srv6 my-sid <prefix> removes entire mySidConfig block"},
    {"deleteMySid_wrongPrefix",
     "delete srv6 my-sid with mismatched prefix is rejected"},
    {"deleteMySid_noConfig",
     "delete srv6 my-sid when nothing configured returns informational message"},
};

class Srv6MySidTestLogListener : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const ::testing::TestInfo& testInfo) override {
    if (!isSrv6MySidTest(testInfo)) {
      return;
    }
    const std::string testName = testInfo.name();
    std::cout << "[TEST] " << testInfo.test_suite_name() << "." << testName;
    const auto description = kSrv6MySidTestDescriptions.find(testName);
    if (description != kSrv6MySidTestDescriptions.end()) {
      std::cout << " | " << description->second;
    }
    std::cout << std::endl;
  }

  void OnTestEnd(const ::testing::TestInfo& testInfo) override {
    if (!isSrv6MySidTest(testInfo)) {
      return;
    }
    const auto* result = testInfo.result();
    std::cout << "[RESULT] " << testInfo.test_suite_name() << "."
              << testInfo.name() << " => "
              << (result->Passed() ? "PASS" : "FAIL") << " ("
              << result->elapsed_time() << " ms)" << std::endl;
  }

 private:
  static bool isSrv6MySidTest(const ::testing::TestInfo& testInfo) {
    return std::string(testInfo.test_suite_name()) ==
        "CmdConfigSrv6MySidTestFixture";
  }
};

struct RegisterSrv6MySidTestLogListener {
  RegisterSrv6MySidTestLogListener() {
    ::testing::UnitTest::GetInstance()->listeners().Append(
        new Srv6MySidTestLogListener());
  }
};

RegisterSrv6MySidTestLogListener registerSrv6MySidTestLogListener;
} // namespace

class CmdConfigSrv6MySidTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigSrv6MySidTestFixture()
      : CmdConfigTestBase(
            "fboss2_config_srv6_mysid_test_%%%%-%%%%-%%%%-%%%%",
            R"({"sw": {}})") {}

 protected:
  void initMySidConfig() {
    setupTestableConfigSession("config srv6 my-sid", kLocatorPrefix);
    CmdConfigSrv6MySid initCmd;
    HostInfo hostInfo("testhost");
    LocatorPrefixArg prefix({kLocatorPrefix});
    initCmd.queryClient(hostInfo, prefix);
  }

  HostInfo hostInfo_{"testhost"};
};

TEST_F(CmdConfigSrv6MySidTestFixture, locatorPrefixArg_valid) {
  LocatorPrefixArg arg({kLocatorPrefix});
  EXPECT_EQ(arg.getPrefix(), kLocatorPrefix);
}

TEST_F(CmdConfigSrv6MySidTestFixture, locatorPrefixArg_notIPv6) {
  EXPECT_THROW(LocatorPrefixArg({"192.168.1.0/24"}), std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, locatorPrefixArg_invalid) {
  EXPECT_THROW(LocatorPrefixArg({"not-a-prefix"}), std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, locatorPrefixArg_unsupportedLength) {
  EXPECT_THROW(LocatorPrefixArg({"3001:db8::/33"}), std::invalid_argument);
  EXPECT_THROW(LocatorPrefixArg({"3001:db8::/120"}), std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, locatorPrefixArg_missing) {
  EXPECT_THROW(LocatorPrefixArg({}), std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidInit_createsConfig) {
  setupTestableConfigSession("config srv6 my-sid", kLocatorPrefix);
  CmdConfigSrv6MySid cmd;
  LocatorPrefixArg prefix({kLocatorPrefix});

  auto result = cmd.queryClient(hostInfo_, prefix);
  EXPECT_THAT(result, HasSubstr("Successfully initialized"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  ASSERT_TRUE(config.sw()->mySidConfig().has_value());
  EXPECT_EQ(*config.sw()->mySidConfig()->locatorPrefix(), kLocatorPrefix);
  EXPECT_TRUE(config.sw()->mySidConfig()->entries()->empty());
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidInit_rejectsSecondInit) {
  initMySidConfig();
  CmdConfigSrv6MySid cmd;
  LocatorPrefixArg prefix({kLocatorPrefix});

  EXPECT_THROW(cmd.queryClient(hostInfo_, prefix), std::runtime_error);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_validAdjacency) {
  MySidAddArg arg(
      {"entry",
       "10188",
       "type",
       "adjacency",
       "is-v6",
       "true",
       "port-name",
       "Port-Channel190412"});
  EXPECT_EQ(arg.getFunctionValue(), 10188);
  EXPECT_EQ(arg.getType(), MySidConfigEntryType::ADJACENCY);
  EXPECT_TRUE(arg.isV6());
  EXPECT_EQ(arg.getPortName(), "Port-Channel190412");
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_validNode) {
  MySidAddArg arg(
      {"entry", "20001", "type", "node", "node-address", "2001:db8::1"});
  EXPECT_EQ(arg.getFunctionValue(), 20001);
  EXPECT_EQ(arg.getType(), MySidConfigEntryType::NODE);
  EXPECT_EQ(arg.getNodeAddress(), "2001:db8::1");
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_validDecap) {
  MySidAddArg arg({"entry", "32767", "type", "decap"});
  EXPECT_EQ(arg.getFunctionValue(), 32767);
  EXPECT_EQ(arg.getType(), MySidConfigEntryType::DECAP);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_noLocatorThrows) {
  setupTestableConfigSession(
      "config srv6 my-sid add",
      fmt::format(
          "{} entry 10188 type adjacency is-v6 true port-name Port-Channel190412",
          kLocatorPrefix));
  CmdConfigSrv6MySidAdd cmd;
  LocatorPrefixArg prefix({kLocatorPrefix});
  MySidAddArg addArg(
      {"entry",
       "10188",
       "type",
       "adjacency",
       "is-v6",
       "true",
       "port-name",
       "Port-Channel190412"});

  EXPECT_THROW(cmd.queryClient(hostInfo_, prefix, addArg), std::runtime_error);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_invalidEntry) {
  EXPECT_THROW(
      MySidAddArg({"entry", "0", "type", "decap"}), std::invalid_argument);
  EXPECT_THROW(
      MySidAddArg({"entry", "99999", "type", "decap"}), std::invalid_argument);
  EXPECT_THROW(
      MySidAddArg({"entry", "abc", "type", "decap"}), std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_unknownType) {
  EXPECT_THROW(
      MySidAddArg({"entry", "1", "type", "binding"}), std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_invalidIsV6) {
  EXPECT_THROW(
      MySidAddArg(
          {"entry",
           "1",
           "type",
           "adjacency",
           "is-v6",
           "maybe",
           "port-name",
           "Port-Channel1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_portNameOnNode) {
  EXPECT_THROW(
      MySidAddArg({"entry", "1", "type", "node", "port-name", "Port-Channel1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_nodeAddressOnAdjacency) {
  EXPECT_THROW(
      MySidAddArg(
          {"entry",
           "1",
           "type",
           "adjacency",
           "is-v6",
           "true",
           "node-address",
           "2001:db8::1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_insertsEntry) {
  initMySidConfig();
  CmdConfigSrv6MySidAdd cmd;
  LocatorPrefixArg prefix({kLocatorPrefix});
  MySidAddArg addArg(
      {"entry",
       "10188",
       "type",
       "adjacency",
       "is-v6",
       "true",
       "port-name",
       "Port-Channel190412"});

  auto result = cmd.queryClient(hostInfo_, prefix, addArg);
  EXPECT_THAT(result, HasSubstr("Successfully added adjacency SID"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  ASSERT_TRUE(config.sw()->mySidConfig().has_value());
  EXPECT_EQ(config.sw()->mySidConfig()->entries()->count(10188), 1);
  EXPECT_TRUE(
      config.sw()->mySidConfig()->entries()->at(10188).adjacency().has_value());
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_prefixMismatch) {
  initMySidConfig();
  CmdConfigSrv6MySidAdd cmd;
  LocatorPrefixArg prefix({kOtherLocatorPrefix});
  MySidAddArg addArg(
      {"entry",
       "10188",
       "type",
       "adjacency",
       "is-v6",
       "true",
       "port-name",
       "Port-Channel190412"});

  EXPECT_THROW(cmd.queryClient(hostInfo_, prefix, addArg), std::runtime_error);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidAdd_upserts) {
  initMySidConfig();
  CmdConfigSrv6MySidAdd cmd;
  LocatorPrefixArg prefix({kLocatorPrefix});
  MySidAddArg firstArg({"entry", "10188", "type", "decap"});
  MySidAddArg secondArg(
      {"entry",
       "10188",
       "type",
       "adjacency",
       "is-v6",
       "true",
       "port-name",
       "Port-Channel190412"});

  cmd.queryClient(hostInfo_, prefix, firstArg);
  auto result = cmd.queryClient(hostInfo_, prefix, secondArg);
  EXPECT_THAT(result, HasSubstr("Warning: MySID entry 10188 overwritten"));
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidDeleteEntry_removesEntry) {
  initMySidConfig();
  CmdConfigSrv6MySidAdd addCmd;
  LocatorPrefixArg prefix({kLocatorPrefix});
  MySidAddArg addArg({"entry", "32767", "type", "decap"});
  addCmd.queryClient(hostInfo_, prefix, addArg);

  CmdConfigSrv6MySidDelete deleteCmd;
  MySidDeleteEntryArg deleteArg({"entry", "32767"});
  auto result = deleteCmd.queryClient(hostInfo_, prefix, deleteArg);
  EXPECT_THAT(result, HasSubstr("Successfully deleted MySID entry 32767"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  EXPECT_EQ(config.sw()->mySidConfig()->entries()->count(32767), 0);
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidDeleteEntry_noOpWhenMissing) {
  initMySidConfig();
  CmdConfigSrv6MySidDelete deleteCmd;
  LocatorPrefixArg prefix({kLocatorPrefix});
  MySidDeleteEntryArg deleteArg({"entry", "10188"});

  auto result = deleteCmd.queryClient(hostInfo_, prefix, deleteArg);
  EXPECT_THAT(result, HasSubstr("does not exist"));
}

TEST_F(CmdConfigSrv6MySidTestFixture, mySidDeleteEntry_noOpWhenNoConfig) {
  setupTestableConfigSession("config srv6 my-sid delete", kLocatorPrefix);
  CmdConfigSrv6MySidDelete deleteCmd;
  LocatorPrefixArg prefix({kLocatorPrefix});
  MySidDeleteEntryArg deleteArg({"entry", "10188"});

  auto result = deleteCmd.queryClient(hostInfo_, prefix, deleteArg);
  EXPECT_THAT(result, HasSubstr("no mySidConfig configured"));
}

TEST_F(CmdConfigSrv6MySidTestFixture, deleteMySid_removesEntireConfig) {
  initMySidConfig();
  CmdDeleteSrv6MySid deleteCmd;
  LocatorPrefixArg prefix({kLocatorPrefix});

  auto result = deleteCmd.queryClient(hostInfo_, prefix);
  EXPECT_THAT(
      result, HasSubstr("Successfully deleted SRv6 MySID configuration"));
  EXPECT_THAT(result, HasSubstr(kLocatorPrefix));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  EXPECT_FALSE(config.sw()->mySidConfig().has_value());
}

TEST_F(CmdConfigSrv6MySidTestFixture, deleteMySid_wrongPrefix) {
  initMySidConfig();
  CmdDeleteSrv6MySid deleteCmd;
  LocatorPrefixArg prefix({kOtherLocatorPrefix});

  EXPECT_THROW(deleteCmd.queryClient(hostInfo_, prefix), std::runtime_error);
}

TEST_F(CmdConfigSrv6MySidTestFixture, deleteMySid_noConfig) {
  setupTestableConfigSession("delete srv6 my-sid", kLocatorPrefix);
  CmdDeleteSrv6MySid deleteCmd;
  LocatorPrefixArg prefix({kLocatorPrefix});

  auto result = deleteCmd.queryClient(hostInfo_, prefix);
  EXPECT_THAT(result, HasSubstr("nothing configured"));
}

} // namespace facebook::fboss
