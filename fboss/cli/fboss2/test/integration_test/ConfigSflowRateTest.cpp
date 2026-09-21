// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config interface <name> sflow ingress-rate <N> egress-rate <N>
 *   fboss2-dev delete interface <name> sflow ingress-rate egress-rate
 *
 * Picks a port, sets both ingress-rate and egress-rate in a single combined
 * command, verifies both round-trip through the agent's running config,
 * deletes both in a single combined command, and verifies the port returns
 * to rate 0 on both.
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration
 *   - Test is run as root (or with sudo) on a DUT
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
constexpr int64_t kTestRate = 256;
} // namespace

class ConfigSflowRateTest : public Fboss2IntegrationTest {
 protected:
  void TearDown() override {
    // If the test failed after committing a non-zero rate but before the
    // delete commit, best-effort clear it so a shared DUT is left clean.
    if (!committedPort_.empty()) {
      try {
        auto config = getRunningConfig();
        if (ingressRateOf(config, committedPort_) != 0) {
          runCli(
              {"delete", "interface", committedPort_, "sflow", "ingress-rate"});
          commitConfig();
        }
        config = getRunningConfig();
        if (egressRateOf(config, committedPort_) != 0) {
          runCli(
              {"delete", "interface", committedPort_, "sflow", "egress-rate"});
          commitConfig();
        }
      } catch (const std::exception&) {
        // Best-effort; do not mask the test result.
      }
    }
    Fboss2IntegrationTest::TearDown();
  }

  std::string committedPort_;

  static int64_t ingressRateOf(
      const folly::dynamic& config,
      const std::string& portName) {
    return rateOf(config, portName, "sFlowIngressRate");
  }

  static int64_t egressRateOf(
      const folly::dynamic& config,
      const std::string& portName) {
    return rateOf(config, portName, "sFlowEgressRate");
  }

  static int64_t rateOf(
      const folly::dynamic& config,
      const std::string& portName,
      const std::string& field) {
    for (const auto& p : config["sw"]["ports"]) {
      if (p.count("name") && p["name"].asString() == portName) {
        return p.count(field) ? p[field].asInt() : 0;
      }
    }
    return 0;
  }

  static std::string pickAnyPort(const folly::dynamic& config) {
    for (const auto& p : config["sw"]["ports"]) {
      if (p.count("name")) {
        return p["name"].asString();
      }
    }
    return "";
  }

  void runCliOkOrDiscard(const std::vector<std::string>& args) {
    auto result = runCli(args);
    if (result.exitCode != 0) {
      discardSession();
    }
    ASSERT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
  }
};

TEST_F(ConfigSflowRateTest, SetThenDeleteIngressAndEgressRateCombined) {
  auto initial = getRunningConfig();
  ASSERT_TRUE(initial.isObject() && initial.count("sw"));

  const std::string portName = pickAnyPort(initial);
  ASSERT_FALSE(portName.empty()) << "no ports in running config";
  committedPort_ = portName;

  XLOG(INFO) << "[Step 1] config interface " << portName
             << " sflow ingress-rate " << kTestRate << " egress-rate "
             << kTestRate << " (combined)";
  runCliOkOrDiscard(
      {"config",
       "interface",
       portName,
       "sflow",
       "ingress-rate",
       std::to_string(kTestRate),
       "egress-rate",
       std::to_string(kTestRate)});
  commitConfig();
  waitForAgentReady();
  {
    auto config = waitForRunningConfig([&](const folly::dynamic& c) {
      return ingressRateOf(c, portName) == kTestRate &&
          egressRateOf(c, portName) == kTestRate;
    });
    ASSERT_EQ(ingressRateOf(config, portName), kTestRate)
        << "ingress-rate not in running config for " << portName;
    ASSERT_EQ(egressRateOf(config, portName), kTestRate)
        << "egress-rate not in running config for " << portName;
  }

  XLOG(INFO) << "[Step 2] delete interface " << portName
             << " sflow ingress-rate egress-rate (combined)";
  runCliOkOrDiscard(
      {"delete",
       "interface",
       portName,
       "sflow",
       "ingress-rate",
       "egress-rate"});
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 3] Verify " << portName << " has rate 0 on both again";
  auto config = waitForRunningConfig([&](const folly::dynamic& c) {
    return ingressRateOf(c, portName) == 0 && egressRateOf(c, portName) == 0;
  });
  EXPECT_EQ(ingressRateOf(config, portName), 0)
      << "ingress-rate still non-zero on " << portName << " after delete";
  EXPECT_EQ(egressRateOf(config, portName), 0)
      << "egress-rate still non-zero on " << portName << " after delete";
}
