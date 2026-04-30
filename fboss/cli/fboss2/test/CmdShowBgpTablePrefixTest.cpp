// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/json.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/reflection/testing.h> // NOLINT(misc-include-cleaner)
#include <initializer_list>
#include <memory>
#include <vector>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTablePrefix.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"
#ifndef IS_OSS
#include "nettools/common/TestUtils.h"
#endif

using namespace ::testing;
using namespace facebook::neteng::fboss::bgp::thrift;
namespace facebook::fboss {
class CmdShowBgpTablePrefixTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<TRibEntry> queriedEntry_;
  const std::string kPrefixToQuery = "8.0.0.0/32";

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    queriedEntry_ = {
        buildEntry(kPrefixToQuery, kNextHop, kPeerId, kPeerDescription)};
  }

  void setupConfig() {
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
  }

  TBgpPath buildFixedPath(
      const std::string& peerId,
      const std::string& nextHop,
      const bool isBestPath = false) {
    // Path created will have peerId as the peerDescription too.
    return buildPath(
        kPrefixToQuery,
        nextHop,
        peerId, // peerId
        peerId, // peerDescription
        true, // setCommunity
        true, // setAsPath
        true, // setExtCommunity
        std::nullopt, // clusterList
        kOriginatorId, // originatorId
        false, // setPolicy
        std::nullopt, // pathNextHopWeight
        isBestPath,
        true, // setMed
        true, // setRecvPathId
        true, // setWeight
        true // setIgpCost
    );
  }
};

TEST_F(CmdShowBgpTablePrefixTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRibPrefix(_, _))
      .WillOnce(Invoke(
          [&](std::vector<TRibEntry>& entries, std::unique_ptr<std::string>) {
            entries = queriedEntry_;
          }));

  auto result =
      CmdShowBgpTablePrefix().queryClient(localhost(), {kPrefixToQuery});
  EXPECT_THRIFT_EQ_VECTOR(*result.tRibEntries(), queriedEntry_);
}

TEST_F(CmdShowBgpTablePrefixTestFixture, queryClientWithInvalidPrefix) {
  const std::string invalidPrefix = "1.1.1.1/32";
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRibPrefix(_, _))
      .WillOnce(Invoke([&](std::vector<TRibEntry>& entries,
                           std::unique_ptr<std::string>) { entries = {}; }));

  auto result =
      CmdShowBgpTablePrefix().queryClient(localhost(), {invalidPrefix});
  EXPECT_THAT(*result.tRibEntries(), IsEmpty());
}

TEST_F(CmdShowBgpTablePrefixTestFixture, printOutput) {
  // Setup.
  setupMockedBgpServer();
  setupConfig();

  // Create TRibEntry (mocked output @queriedEntry_) from getRibEntries.
  TRibEntryWithHost tRibEntryWithHost;
  tRibEntryWithHost.tRibEntries() = queriedEntry_;
  tRibEntryWithHost.host() = localhost().getName();
  tRibEntryWithHost.oobName() = localhost().getOobName();
  tRibEntryWithHost.ip() = localhost().getIpStr();

  std::stringstream ss;
  CmdShowBgpTablePrefix().printOutput(tRibEntryWithHost, ss);
  std::string output = ss.str();
  // Expect one selected path.
  std::string expectedOutput = kRibEntryMarkersHeader +
      "\n> 8.0.0.0/32, Selected 1/1 paths\n"
      "*@ from 1.2.3.4 (one.two.three.four) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: [1.1.1.2]\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n";

  maskDateInOutput(output);
  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpTablePrefixTestFixture, PrintOutput_OnlyDefaultPaths) {
  // Setup.
  setupMockedBgpServer();
  setupConfig();

  // Create TRibEntry with 2 default paths.
  TRibEntryWithHost tRibEntryWithHost;
  TRibEntry entry = buildEntry(
      kPrefixToQuery,
      kNextHop, // nextHop
      {
          {kDefaultGroupName,
           {
               buildFixedPath(
                   "1.2.3.2", // peerId
                   kNextHop),
               buildFixedPath(
                   "1.2.3.3", // peerId
                   kNextHop),
           }},
      }, // paths map
      true /* omitBestPath */);
  tRibEntryWithHost.tRibEntries()->push_back(std::move(entry));
  tRibEntryWithHost.host() = localhost().getName();
  tRibEntryWithHost.oobName() = localhost().getOobName();
  tRibEntryWithHost.ip() = localhost().getIpStr();

  // Show output.
  std::stringstream ss;
  CmdShowBgpTablePrefix().printOutput(tRibEntryWithHost, ss);
  std::string output = ss.str();

  // No paths should be marked with * or @ as we do not have any best paths.
  std::string expectedOutput = kRibEntryMarkersHeader +
      "\n> 8.0.0.0/32, Selected 0/2 paths\n"
      // path 1
      "   from 1.2.3.2 (1.2.3.2) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: []\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n"
      // path 2
      "   from 1.2.3.3 (1.2.3.3) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: []\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n";

  maskDateInOutput(output);
  EXPECT_EQ(output, expectedOutput);
}

TEST_F(
    CmdShowBgpTablePrefixTestFixture,
    PrintOutput_SelectBestPathWithIsBestPath) {
  // Setup.
  setupMockedBgpServer();
  setupConfig();

  // Create TRibEntry with 3 paths; 2 as best paths, 1 as default path.
  TRibEntryWithHost tRibEntryWithHost;
  TRibEntry entry = buildEntry(
      kPrefixToQuery,
      kNextHop, // nextHop
      {
          {kBestGroupName,
           {
               buildFixedPath(
                   "1.2.3.2", // peerId
                   kNextHop,
                   true), // isBestPath
               buildFixedPath("1.2.3.3" /* peerId */, kNextHop),
           }},
          {kDefaultGroupName,
           {
               buildFixedPath("1.2.3.4" /* peerId */, kNextHop),
           }},
      }, // paths map
      false); // omitBestPath
  tRibEntryWithHost.tRibEntries()->push_back(std::move(entry));
  tRibEntryWithHost.host() = localhost().getName();
  tRibEntryWithHost.oobName() = localhost().getOobName();
  tRibEntryWithHost.ip() = localhost().getIpStr();

  // Show output.
  std::stringstream ss;
  CmdShowBgpTablePrefix().printOutput(tRibEntryWithHost, ss);
  std::string output = ss.str();

  // When is_best_path is set on TBgpPath, we indicate this path is selected.
  // Only 1.2.3.2 should be selected as the very best entry (@).
  std::string expectedOutput = kRibEntryMarkersHeader +
      "\n> 8.0.0.0/32, Selected 2/3 paths\n"
      // SELECTED path 1 (best path group)
      "*@ from 1.2.3.2 (1.2.3.2) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: []\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n"
      // path 2 (best path group)
      "*  from 1.2.3.3 (1.2.3.3) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: []\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n"
      // path 3 (default path group)
      "   from 1.2.3.4 (1.2.3.4) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: []\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n";

  maskDateInOutput(output);
  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowBgpTablePrefixTestFixture, PrintOutput_PathSelectionPending) {
  // Test that path_selection_pending indicator is displayed when set
  setupMockedBgpServer();
  setupConfig();

  // Create TRibEntry with path_selection_pending = true
  TRibEntryWithHost tRibEntryWithHost;
  TRibEntry entry =
      buildEntry(kPrefixToQuery, kNextHop, kPeerId, kPeerDescription);
  entry.path_selection_pending() = true;
  tRibEntryWithHost.tRibEntries()->push_back(std::move(entry));
  tRibEntryWithHost.host() = localhost().getName();
  tRibEntryWithHost.oobName() = localhost().getOobName();
  tRibEntryWithHost.ip() = localhost().getIpStr();

  std::stringstream ss;
  CmdShowBgpTablePrefix().printOutput(tRibEntryWithHost, ss);
  std::string output = ss.str();

  // Verify that the % marker is in the output when path selection is pending.
  // This indicates the path-selection results displayed are in a transient
  // state - wait for the final result after the marker is cleared.
  std::string expectedOutput = kRibEntryMarkersHeader +
      "\n>% 8.0.0.0/32, Selected 1/1 paths\n"
      "*@ from 1.2.3.4 (one.two.three.four) via 8.0.0.1 | LBW: None | Origin: INCOMPLETE | LP: DEPRIO/25 | ASP: 65301 | LM: # | NH Weight: N/A | MED: 10 | ID: 5 (rcvd) 6 (sent) | Weight: 20 | IgpCost: 100"
      "\n    Router/Originator: 2.2.2.3 | ClusterList: [1.1.1.2]\n"
      "    Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "    ExtCommunities: Type(64):SubType(2):AS(3):Value(4)\n"
      "    BestPath Rejection Reason: Router-Id, Filter Criterion: Choose Lowest Value\n";

  maskDateInOutput(output);
  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
