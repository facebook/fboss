// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/reflection/testing.h> // NOLINT(misc-include-cleaner)
#include <vector>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include <folly/json/json.h>

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTableDetail.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"
#ifndef IS_OSS
#include "nettools/common/TestUtils.h"
#endif

using namespace ::testing;
using namespace facebook::neteng::fboss::bgp::thrift;
namespace facebook::fboss {
class CmdShowBgpTableDetailTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<TRibEntry> entriesIPv4_;
  std::vector<TRibEntry> entriesIPv6_;
  std::vector<TRibEntry> combinedEntries_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    entriesIPv4_ = {buildEntry()};
    entriesIPv6_ = {
        buildEntry("2001::1/64", "2001::2", "2001::3", "two00one::three", 7)};
    combineEntries();
  }

  void combineEntries() {
    this->combinedEntries_ = this->entriesIPv4_;
    this->combinedEntries_.insert(
        combinedEntries_.end(), entriesIPv6_.begin(), entriesIPv6_.end());
  }
};

TEST_F(CmdShowBgpTableDetailTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRibEntries(_, TBgpAfi::AFI_IPV4))
      .WillOnce(Invoke([&](std::vector<TRibEntry>& entries, TBgpAfi) {
        entries = entriesIPv4_;
      }));

  EXPECT_CALL(getMockBgp(), getRibEntries(_, TBgpAfi::AFI_IPV6))
      .WillOnce(Invoke([&](std::vector<TRibEntry>& entries, TBgpAfi) {
        entries = entriesIPv6_;
      }));
  auto result = CmdShowBgpTableDetail().queryClient(localhost());
  EXPECT_THRIFT_EQ_VECTOR(*result.tRibEntries(), combinedEntries_);
}

TEST_F(CmdShowBgpTableDetailTestFixture, printOutput) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .WillRepeatedly(Invoke([&](std::string& config) {
        // clang-format off
        folly::dynamic value = folly::dynamic::object
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
        config = folly::toPrettyJson(value);
      }));
  std::stringstream ss;
  TRibEntryWithHost tRibEntryWithHost;
  tRibEntryWithHost.tRibEntries() = combinedEntries_;
  tRibEntryWithHost.host() = localhost().getName();
  tRibEntryWithHost.oobName() = localhost().getOobName();
  tRibEntryWithHost.ip() = localhost().getIpStr();
  CmdShowBgpTableDetail().printOutput(tRibEntryWithHost, ss);
  std::string output = ss.str();

  std::string expectedOutput = kRibEntryMarkersHeader +
      "\n> 8.0.0.0/32, Selected 1/1 paths\n"
      "*@ from 1.2.3.4 (one.two.three.four) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: [1.1.1.2]\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n"
      "\n> 2001::1/64, Selected 1/1 paths\n"
      "*@ from 2001::3 (two00one::three) via 2001::2 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: 7 | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: [1.1.1.2]\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n";

  maskDateInOutput(output);
  EXPECT_EQ(output, expectedOutput);
}
} // namespace facebook::fboss
