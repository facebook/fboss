// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"

using namespace ::testing;
using namespace facebook::fboss;

struct SpeedAndProfile {
  cfg::PortSpeed speed;
  cfg::PortProfileID profileID;
  SpeedAndProfile(cfg::PortSpeed _speed, cfg::PortProfileID _profileID)
      : speed(_speed), profileID(_profileID) {}
};

class SpeedChangeTest : public LinkTest {
 public:
  void TearDown() override;
  void setupConfigFlag() override;

 protected:
  std::optional<SpeedAndProfile> getSecondarySpeedAndProfile(
      cfg::PortProfileID profileID) const;

 private:
  std::string originalConfigCopy;
};

void SpeedChangeTest::setupConfigFlag() {
  // Coldboot iteration -
  //    SetUp -
  //      1. Make a copy of the original config. Existing copy is overridden
  //      2. The speeds and profiles are modified by this test in the original
  //      config.
  //    TearDown - Do nothing. Keep the modified config in the FLAGS_config
  //    path.
  // Warmboot iteration -
  //    SetUp - Do nothing. Agent loads up with the modified config (setup by
  //      coldboot iteration)
  //    TearDown - Copy is deleted if we are not asked to setup_for_warmboot
  //    again
  auto linkTestDir = FLAGS_volatile_state_dir + "/link_test";
  originalConfigCopy =
      linkTestDir + "/agent_speed_change_test_original_config.conf";

  // Create a copy of the original config if we haven't created one before.
  if (!boost::filesystem::exists(originalConfigCopy)) {
    // Likely a coldboot iteration
    utilCreateDir(linkTestDir);
    boost::filesystem::copy_file(FLAGS_config, originalConfigCopy);
  }
  // Make sure the copy exists
  CHECK(boost::filesystem::exists(originalConfigCopy));
}

void SpeedChangeTest::TearDown() {
  if (!FLAGS_setup_for_warmboot) {
    // If this test wasn't setup for warmboot, revert back the original config
    // in FLAGS_config path. Also clean up the copy
    boost::filesystem::copy_file(
        originalConfigCopy,
        FLAGS_config,
        boost::filesystem::copy_option::overwrite_if_exists);
    CHECK(removeFile(originalConfigCopy));
  }
  LinkTest::TearDown();
}

// Returns a secondary speed if the platform supports it
std::optional<SpeedAndProfile> SpeedChangeTest::getSecondarySpeedAndProfile(
    cfg::PortProfileID profileID) const {
  if (profileID == cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL) {
    return SpeedAndProfile(
        cfg::PortSpeed::HUNDREDG,
        cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL);
  } else if (
      profileID == cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL) {
    return SpeedAndProfile(
        cfg::PortSpeed::TWOHUNDREDG,
        cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL);
  }
  return std::nullopt;
}

// Configures secondary speeds (downgrade speed) and ensure link sanity
TEST_F(SpeedChangeTest, secondarySpeed) {
  auto speedChangeSetup = [this]() {
    // Create a new config with secondary speed
    cfg::AgentConfig testConfig = platform()->config()->thrift;
    auto swConfig = *testConfig.sw();
    bool speedChanged = false;
    for (auto& port : *swConfig.ports()) {
      if (auto speedAndProfile =
              getSecondarySpeedAndProfile(*port.profileID())) {
        XLOG(INFO) << folly::sformat(
            "Changing speed and profile on port {:s} from speed={:s},profile={:s} to speed={:s},profile={:s}",
            *port.name(),
            apache::thrift::util::enumName(*port.speed()),
            apache::thrift::util::enumName(*port.profileID()),
            apache::thrift::util::enumName(speedAndProfile->speed),
            apache::thrift::util::enumName(speedAndProfile->profileID));
        port.speed() = speedAndProfile->speed;
        port.profileID() = speedAndProfile->profileID;
        speedChanged = true;
      }
    }
    CHECK(speedChanged);
    *testConfig.sw() = swConfig;

    // Dump the new config to the config file
    auto newcfg = AgentConfig(
        testConfig,
        apache::thrift::SimpleJSONSerializer::serialize<std::string>(
            testConfig));
    newcfg.dumpConfig(FLAGS_config);

    // Apply the new config
    sw()->applyConfig("set secondary speeds", true);

    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    createL3DataplaneFlood();
  };
  auto speedChangeVerify = [this]() {
    /*
     * Verify the following on all cabled ports
     * 1. Link comes up at secondary speeds
     * 2. LLDP neighbor is discovered
     * 3. Assert no in discards
     */
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    checkWithRetry(
        [this]() { return sendAndCheckReachabilityOnAllCabledPorts(); });
    // Assert no traffic loss and no ecmp shrink. If ports flap
    // these conditions will not be true
    assertNoInDiscards();
    auto ecmpSizeInSw = getVlanOwningCabledPorts().size();
    EXPECT_EQ(
        utility::getEcmpSizeInHw(
            sw()->getHw(),
            {folly::IPAddress("::"), 0},
            RouterID(0),
            ecmpSizeInSw),
        ecmpSizeInSw);
  };

  verifyAcrossWarmBoots(speedChangeSetup, speedChangeVerify);
}
