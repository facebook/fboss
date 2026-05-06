// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config vlan <id> port <port> taggingMode
 *     (tagged | untagged | priority-tagged)
 *
 * Walks the port through tagged -> priority-tagged -> untagged, verifying
 * each transition against the agent's running config, then restores the
 * original mode.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigVlanPortTaggingModeTest : public Fboss2IntegrationTest {
 protected:
  std::string getTaggingMode(int vlanId, const std::string& portName) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];

    std::map<int, std::string> logicalToName;
    if (sw.count("ports")) {
      for (const auto& port : sw["ports"]) {
        if (port.count("logicalID") && port.count("name")) {
          logicalToName[port["logicalID"].asInt()] = port["name"].asString();
        }
      }
    }

    if (!sw.count("vlanPorts")) {
      return "";
    }
    for (const auto& vp : sw["vlanPorts"]) {
      if (vp["vlanID"].asInt() != vlanId) {
        continue;
      }
      auto it = logicalToName.find(vp["logicalPort"].asInt());
      if (it == logicalToName.end() || it->second != portName) {
        continue;
      }
      bool emitTags = vp.getDefault("emitTags", false).asBool();
      bool emitPriorityTags = vp.getDefault("emitPriorityTags", false).asBool();
      if (emitPriorityTags) {
        return "priority-tagged";
      }
      return emitTags ? "tagged" : "untagged";
    }
    return "";
  }

  void setTaggingMode(
      int vlanId,
      const std::string& portName,
      const std::string& mode) {
    auto result = runCli(
        {"config",
         "vlan",
         std::to_string(vlanId),
         "port",
         portName,
         "taggingMode",
         mode});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set tagging mode " << mode << ": " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigVlanPortTaggingModeTest, TransitionsBetweenAllModes) {
  XLOG(INFO) << "[Step 1] Finding a (VLAN, port) pair with membership...";
  auto vp = findConfiguredVlanPort();
  if (!vp.has_value()) {
    GTEST_SKIP()
        << "This DUT's running config has no port that is a member of a "
           "configured VLAN via sw.vlanPorts; the 'config vlan <id> port "
           "<port> taggingMode' command requires such a pairing.";
  }
  int vlanId = vp->first;
  const std::string& portName = vp->second;
  // Keep the 'intf' variable for symmetry with the rest of the file.
  Interface intf;
  intf.name = portName;
  intf.vlan = vlanId;
  XLOG(INFO) << "  Using " << intf.name << " on VLAN " << vlanId;

  auto originalMode = getTaggingMode(vlanId, intf.name);
  if (originalMode.empty()) {
    originalMode = "untagged";
  }
  XLOG(INFO) << "  Original tagging mode: " << originalMode;

  XLOG(INFO) << "[Step 2] Setting tagging mode to 'tagged'...";
  setTaggingMode(vlanId, intf.name, "tagged");
  EXPECT_EQ(getTaggingMode(vlanId, intf.name), "tagged");

  XLOG(INFO) << "[Step 3] Setting tagging mode to 'priority-tagged'...";
  setTaggingMode(vlanId, intf.name, "priority-tagged");
  EXPECT_EQ(getTaggingMode(vlanId, intf.name), "priority-tagged");

  XLOG(INFO) << "[Step 4] Setting tagging mode to 'untagged'...";
  setTaggingMode(vlanId, intf.name, "untagged");
  EXPECT_EQ(getTaggingMode(vlanId, intf.name), "untagged");

  if (originalMode != "untagged") {
    XLOG(INFO) << "[Cleanup] Restoring original mode '" << originalMode
               << "'...";
    setTaggingMode(vlanId, intf.name, originalMode);
  }

  XLOG(INFO) << "TEST PASSED";
}
