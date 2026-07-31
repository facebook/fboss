// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for `fboss2-dev delete arp <attr> [<attr> ...]`.
 *
 * Covers resetting the 4 hitless ARP/NDP tunables to their
 * switch_config.thrift defaults:
 *   - timeout         -> arpTimeoutSeconds (60)
 *   - age-interval    -> arpAgerInterval (5)
 *   - max-probes      -> maxNeighborProbes (300)
 *   - stale-interval  -> staleEntryInterval (10)
 *
 * Per-attribute dispatch and argument validation are covered by unit tests
 * (CmdDeleteArpTest.cpp); this test verifies the full stack once:
 *   1. Set a non-default value for every attr via `config arp` + commit
 *   2. Reset all four with a single `delete arp` invocation
 *   3. Commit (HITLESS — no agent restart)
 *   4. Verify running config + switch state reflect the thrift defaults
 *   5. Restore original values
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration
 *   - Test is run as root (or with sudo) on a DUT
 */

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
struct ArpAttr {
  std::string attr;
  std::string swField;
  std::string switchStateField;
  int64_t thriftDefault;
  int64_t nonDefault;
};

const std::vector<ArpAttr> kArpAttrs = {
    {"timeout", "arpTimeoutSeconds", "arpTimeout", 60, 75},
    {"age-interval", "arpAgerInterval", "arpAgerInterval", 5, 8},
    {"max-probes", "maxNeighborProbes", "maxNeighborProbes", 300, 250},
    {"stale-interval", "staleEntryInterval", "staleEntryInterval", 10, 15},
};
} // namespace

class DeleteArpTest : public Fboss2IntegrationTest {};

TEST_F(DeleteArpTest, ResetAllAttrsAtOnce) {
  // Set every attr to a non-default value, then reset all four with a
  // single `delete arp` invocation and one commit.
  std::vector<int64_t> originals;
  for (const auto& a : kArpAttrs) {
    originals.push_back(getSwConfigField<int64_t>(a.swField));
    auto result =
        runCli({"config", "arp", a.attr, std::to_string(a.nonDefault)});
    ASSERT_EQ(result.exitCode, 0) << result.stderr;
  }
  commitConfig();
  for (const auto& a : kArpAttrs) {
    ASSERT_EQ(getSwConfigField<int64_t>(a.swField), a.nonDefault);
  }

  auto result = runCli(
      {"delete",
       "arp",
       "timeout",
       "age-interval",
       "max-probes",
       "stale-interval"});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  commitConfig();

  for (const auto& a : kArpAttrs) {
    EXPECT_EQ(getSwConfigField<int64_t>(a.swField), a.thriftDefault)
        << a.swField;
    verifySwitchSettingsField<int64_t>(a.switchStateField, a.thriftDefault);
  }

  for (size_t i = 0; i < kArpAttrs.size(); ++i) {
    auto restore = runCli(
        {"config", "arp", kArpAttrs[i].attr, std::to_string(originals[i])});
    ASSERT_EQ(restore.exitCode, 0) << restore.stderr;
  }
  commitConfig();
  for (size_t i = 0; i < kArpAttrs.size(); ++i) {
    EXPECT_EQ(getSwConfigField<int64_t>(kArpAttrs[i].swField), originals[i]);
  }
}
