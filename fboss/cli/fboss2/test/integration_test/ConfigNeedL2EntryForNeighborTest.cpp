/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * End-to-end test for:
 *   fboss2-dev config need-l2-entry-for-neighbor <enabled|disabled>
 *
 * Writes the config field and verifies the value round-trips through the
 * agent's running config. SwitchSettings::needL2EntryForNeighbor is a
 * config-only field: it is not part of switch_state.thrift, ApplyThriftConfig
 * does not apply it, and the HwSwitchHandler implementations derive the value
 * from ASIC type. This test therefore validates the CLI write path only
 * (CLI -> config session commit -> agent reload -> running config).
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigNeedL2EntryForNeighborTest : public Fboss2IntegrationTest {
 protected:
  std::optional<bool> getNeedL2EntryForNeighbor() const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("switchSettings")) {
      return std::nullopt;
    }
    const auto& settings = sw["switchSettings"];
    if (!settings.count("needL2EntryForNeighbor")) {
      return std::nullopt;
    }
    return settings["needL2EntryForNeighbor"].asBool();
  }

  void setNeedL2EntryForNeighbor(const std::string& state) {
    auto result = runCli({"config", "need-l2-entry-for-neighbor", state});
    ASSERT_EQ(result.exitCode, 0)
        << "need-l2-entry-for-neighbor CLI failed: " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigNeedL2EntryForNeighborTest, ToggleNeedL2EntryForNeighbor) {
  XLOG(INFO) << "[Step 1] Reading current need-l2-entry-for-neighbor value...";
  bool original = getNeedL2EntryForNeighbor().value_or(true);
  XLOG(INFO) << "  Current: " << (original ? "enabled" : "disabled");

  std::string first = original ? "disabled" : "enabled";
  std::string second = original ? "enabled" : "disabled";

  XLOG(INFO) << "[Step 2] Setting to '" << first << "'...";
  setNeedL2EntryForNeighbor(first);
  auto after1 = getNeedL2EntryForNeighbor();
  ASSERT_TRUE(after1.has_value());
  EXPECT_EQ(*after1, !original);

  XLOG(INFO) << "[Step 3] Restoring to '" << second << "'...";
  setNeedL2EntryForNeighbor(second);
  auto after2 = getNeedL2EntryForNeighbor();
  ASSERT_TRUE(after2.has_value());
  EXPECT_EQ(*after2, original);

  XLOG(INFO) << "TEST PASSED";
}
