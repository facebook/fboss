// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/json.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/reflection/testing.h> // NOLINT(misc-include-cleaner)
#include <vector>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h" // NOLINT(misc-include-cleaner)
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/changelist/CmdShowBgpChangelist.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"
#ifndef IS_OSS
#include "nettools/common/TestUtils.h"
#endif

using facebook::neteng::fboss::bgp::thrift::TRibEntry;
using facebook::neteng::fboss::bgp::thrift::TRibEntryWithHost;
using facebook::neteng::fboss::bgp_attr::TBgpAfi;
using ::testing::_;
using ::testing::Invoke;

namespace facebook::fboss {
class CmdShowBgpChangelistTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<TRibEntry> entriesIPv4_;
  std::vector<TRibEntry> entriesIPv6_;
  std::vector<TRibEntry> combinedEntries_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    entriesIPv4_ = {buildEntry()};
    entriesIPv6_ = {
        buildEntry("2001::1/64", "2001::2", "2001::3", "two00one::three")};
    combineEntries();
  }

  void combineEntries() {
    this->combinedEntries_ = this->entriesIPv4_;
    this->combinedEntries_.insert(
        combinedEntries_.end(), entriesIPv6_.begin(), entriesIPv6_.end());
  }
};

TEST_F(CmdShowBgpChangelistTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getChangeListEntries(_, TBgpAfi::AFI_IPV4))
      .WillOnce(Invoke([&](std::vector<TRibEntry>& entries, TBgpAfi) {
        entries = entriesIPv4_;
      }));

  EXPECT_CALL(getMockBgp(), getChangeListEntries(_, TBgpAfi::AFI_IPV6))
      .WillOnce(Invoke([&](std::vector<TRibEntry>& entries, TBgpAfi) {
        entries = entriesIPv6_;
      }));
  auto result = CmdShowBgpChangelist().queryClient(localhost());
  EXPECT_THRIFT_EQ_VECTOR(*result.tRibEntries(), combinedEntries_);
}

TEST_F(CmdShowBgpChangelistTestFixture, printOutput) {
  combinedEntries_.emplace_back(
      buildEntry("8.0.0.1/32", "8.0.0.2", "8.1.2.8", "eight.one.two.eight"));
  combinedEntries_.emplace_back(
      buildEntry("2001::3/64", "2001::4", "2001::5", "two00one::five"));

  // clang-format off
  folly::dynamic cpsConfigValue = folly::dynamic::object
    ("communities",
    folly::dynamic::array(
    folly::dynamic::object("name", "FABRIC_POD_RSW_LOOP")
    ("description", "rsw loopback")
    ("communities", folly::dynamic::array("65527:12705"))
    )
  )
  ("localprefs",
  folly::dynamic::array(
    folly::dynamic::object("localpref", 20)
    ("name", "LOCALPREF_CTRL_BACKUP")
    ("description", "low-priority supplementary/backup routes from bgp controller"),
    folly::dynamic::object("localpref", 25)
    ("name", "LOCALPREF_DEPRIO")
    ("description", "deprioritized local preference value"))
  );
  // clang-format on

  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .WillRepeatedly(Invoke([cpsConfigValue](std::string& config) {
        config = folly::toPrettyJson(cpsConfigValue);
      }));
  std::stringstream ss;
  TRibEntryWithHost tRibEntryWithHost;
  tRibEntryWithHost.tRibEntries() = combinedEntries_;
  tRibEntryWithHost.host() = localhost().getName();
  tRibEntryWithHost.oobName() = localhost().getOobName();
  tRibEntryWithHost.ip() = localhost().getIpStr();
  CmdShowBgpChangelist().printOutput(tRibEntryWithHost, ss);
  std::string output = ss.str();

  std::string expectedOutput = kRibEntryMarkersHeader +
      "\n> 8.0.0.0/32, Selected 1/1 paths\n"
      "*@ from 1.2.3.4 (one.two.three.four) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n"
      "\n> 8.0.0.1/32, Selected 1/1 paths\n"
      "*@ from 8.1.2.8 (eight.one.two.eight) via 8.0.0.2 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n"
      "\n> 2001::1/64, Selected 1/1 paths\n"
      "*@ from 2001::3 (two00one::three) via 2001::2 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n"
      "\n> 2001::3/64, Selected 1/1 paths\n"
      "*@ from 2001::5 (two00one::five) via 2001::4 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n";

  maskDateInOutput(output);
  EXPECT_EQ(output, expectedOutput);
}

#ifndef IS_OSS
// Test the CPS annotation (Meta-internal feature, not available in OSS)
TEST_F(CmdShowBgpChangelistTestFixture, printCPSOutput) {
  // PathSelector contains the info for the active criteria
  bgp::rib_policy::TPathSelector activeCriteria1;
  activeCriteria1.bgp_native_path_selection_min_nexthop() = 20;
  auto pathOverriddenEntry1 = buildEntry(
      "8.0.0.1/32",
      "8.0.0.2",
      "8.1.2.8",
      "eight.one.two.eight",
      std::nullopt,
      true /* omit best path */,
      true /* omit best paths */);
  pathOverriddenEntry1.active_cps_criteria() = std::move(activeCriteria1);
  combinedEntries_.push_back(std::move(pathOverriddenEntry1));
  auto pathOverriddenEntry3 = buildEntry(
      "8.0.0.3/32",
      "8.0.0.2",
      "8.1.2.8",
      "eight.one.two.eight",
      std::nullopt,
      true /* omit best path */);
  bgp::rib_policy::TPathSelector activeCriteria3;
  activeCriteria3.bgp_native_path_selection_min_nexthop() = 20;
  activeCriteria3.relax_bgp_native_path_selection_min_nexthop() = true;
  pathOverriddenEntry3.active_cps_criteria() = std::move(activeCriteria3);
  combinedEntries_.push_back(std::move(pathOverriddenEntry3));

  bgp::rib_policy::TPathSelector activeCriteria2;
  bgp::rib_policy::TPathSelectionCriteria tCriteria;
  tCriteria.min_nexthop() = 13;
  activeCriteria2.criteria_list()->push_back(std::move(tCriteria));
  auto pathOverriddenEntry2 =
      buildEntry("2001::3/64", "2001::4", "2001::5", "two00one::five");
  pathOverriddenEntry2.active_cps_criteria() = std::move(activeCriteria2);
  combinedEntries_.push_back(std::move(pathOverriddenEntry2));
  // clang-format off
  folly::dynamic cpsConfigValue2 = folly::dynamic::object
    ("communities",
    folly::dynamic::array(
    folly::dynamic::object("name", "FABRIC_POD_RSW_LOOP")
    ("description", "rsw loopback")
    ("communities", folly::dynamic::array("65527:12705"))
    )
  )
  ("localprefs",
  folly::dynamic::array(
    folly::dynamic::object("localpref", 20)
    ("name", "LOCALPREF_CTRL_BACKUP")
    ("description", "low-priority supplementary/backup routes from bgp controller"),
    folly::dynamic::object("localpref", 25)
    ("name", "LOCALPREF_DEPRIO")
    ("description", "deprioritized local preference value"))
  );
  // clang-format on

  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .WillRepeatedly(Invoke([cpsConfigValue2](std::string& config) {
        config = folly::toPrettyJson(cpsConfigValue2);
      }));
  std::stringstream ss;
  TRibEntryWithHost tRibEntryWithHost;
  tRibEntryWithHost.tRibEntries() = combinedEntries_;
  tRibEntryWithHost.host() = localhost().getName();
  tRibEntryWithHost.oobName() = localhost().getOobName();
  tRibEntryWithHost.ip() = localhost().getIpStr();
  CmdShowBgpChangelist().printOutput(tRibEntryWithHost, ss);
  std::string output = ss.str();

  std::string expectedOutput = kRibEntryMarkersHeader +
      "\n> 8.0.0.0/32, Selected 1/1 paths\n"
      "*@ from 1.2.3.4 (one.two.three.four) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n"
      "\n> 8.0.0.1/32, Selected 0/1 paths\n"
      "Path overridden by CPS:\n"
      "  prefix: 8.0.0.1/32\n"
      "    default BGP multipath selector\n"
      "    BGP native min nexthop: 20\n"
      "   from 8.1.2.8 (eight.one.two.eight) via 8.0.0.2 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n"
      "\n> 8.0.0.3/32, Selected 1/1 paths\n"
      "Path overridden by CPS:\n"
      "  prefix: 8.0.0.3/32\n"
      "    default BGP multipath selector\n"
      "    BGP native min nexthop: 20, relaxed: true\n"
      "*  from 8.1.2.8 (eight.one.two.eight) via 8.0.0.2 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n"
      "\n> 2001::1/64, Selected 1/1 paths\n"
      "*@ from 2001::3 (two00one::three) via 2001::2 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n"
      "\n> 2001::3/64, Selected 1/1 paths\n"
      "Path overridden by CPS:\n"
      "  prefix: 2001::3/64\n"
      "    active criteria:\n"
      "      path_matchers:\n"
      "      min nexthop: 13\n"
      "*@ from 2001::5 (two00one::five) via 2001::4 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100\n";

  maskDateInOutput(output);
  EXPECT_EQ(output, expectedOutput);
}
#endif // IS_OSS
} // namespace facebook::fboss
