// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/link_tests/AgentEnsembleLinkTest.h"
#include "fboss/agent/test/link_tests/LinkTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

using namespace ::testing;
using namespace facebook::fboss;

struct SpeedAndProfile {
  cfg::PortSpeed speed;
  cfg::PortProfileID profileID;
  SpeedAndProfile(cfg::PortSpeed _speed, cfg::PortProfileID _profileID)
      : speed(_speed), profileID(_profileID) {}
};

class AgentEnsembleSpeedChangeTest : public AgentEnsembleLinkTest {
 public:
  void TearDown() override;
  void preInitSetup() override;
  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) override;

 protected:
  // Query Qsfp service for eligible port profiles. If current port speed is the
  // same as from_speed and there exists eligible port profiles for to_speed,
  // return that info to the caller.
  std::map<int, cfg::PortProfileID> getEligibleOpticalPortsAndProfile(
      cfg::SwitchConfig cfg,
      cfg::PortSpeed fromSpeed,
      cfg::PortSpeed toSpeed) {
    std::map<std::string, std::vector<cfg::PortProfileID>> supportedProfiles;
    auto qsfpServiceClient = utils::createQsfpServiceClient();
    qsfpServiceClient->sync_getAllPortSupportedProfiles(
        supportedProfiles, true /* checkOptics */);

    std::vector<int32_t> transceiverIds;
    // Get all transceiverIDs for the ports
    for (const auto& port : *cfg.ports()) {
      PortID portID = PortID(*port.logicalID());
      auto tcvrId =
          getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(portID);
      transceiverIds.push_back(tcvrId);
    }

    auto transceiverInfos = utility::waitForTransceiverInfo(transceiverIds);
    std::map<int, cfg::PortProfileID> eligiblePortsAndProfile;
    for (const auto& port : *cfg.ports()) {
      CHECK(port.name().has_value());
      auto tcvrId = getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(
          PortID(*port.logicalID()));
      auto tcvrInfo = transceiverInfos.find(tcvrId);
      if (tcvrInfo != transceiverInfos.end()) {
        auto tcvrState = *tcvrInfo->second.tcvrState();
        if (TransmitterTechnology::OPTICAL ==
            tcvrState.cable().value_or({}).transmitterTech()) {
          // Only consider optical ports
          if (*port.speed() == fromSpeed &&
              supportedProfiles.find(*port.name()) != supportedProfiles.end()) {
            for (const auto& profile : supportedProfiles.at(*port.name())) {
              if (utility::getSpeed(profile) == toSpeed) {
                eligiblePortsAndProfile.emplace(*port.logicalID(), profile);
                break;
              }
            }
          }
        }
      }
    }
    return eligiblePortsAndProfile;
  }

  cfg::SwitchConfig createSpeedChangeConfig(
      cfg::PortSpeed fromSpeed,
      cfg::PortSpeed toSpeed) {
    cfg::AgentConfig testConfig = getSw()->getAgentConfig();
    auto swConfig = *testConfig.sw();

    // Iterate through ports to find eligible ports
    auto eligilePortsAndProfile =
        getEligibleOpticalPortsAndProfile(swConfig, fromSpeed, toSpeed);
    CHECK_GT(eligilePortsAndProfile.size(), 0);

    for (auto& port : *swConfig.ports()) {
      auto iter = eligilePortsAndProfile.find(*port.logicalID());
      if (iter != eligilePortsAndProfile.end()) {
        auto desiredProfileId = iter->second;
        XLOG(INFO) << folly::sformat(
            "Changing speed and profile on port {:s} from speed={:s},profile={:s} to speed={:s},profile={:s}",
            port.name().ensure(),
            apache::thrift::util::enumName(*port.speed()),
            apache::thrift::util::enumName(*port.profileID()),
            apache::thrift::util::enumName(utility::getSpeed(desiredProfileId)),
            apache::thrift::util::enumName(desiredProfileId));
        PortID portID = PortID(*port.logicalID());
        utility::configurePortProfile(
            getAgentEnsemble()->getPlatformMapping(),
            getAgentEnsemble()->getSw()->getPlatformSupportsAddRemovePort(),
            swConfig,
            desiredProfileId,
            utility::getAllPortsInGroup(getSw()->getPlatformMapping(), portID),
            portID);
      }
    }

    return swConfig;
  }

  std::optional<SpeedAndProfile> getSecondarySpeedAndProfile(
      cfg::PortProfileID profileID) const;

  void createSecondarySpeedConfig() {
    cfg::AgentConfig testConfig = getSw()->getAgentConfig();
    auto swConfig = *testConfig.sw();
    bool speedChanged = false;
    for (auto& port : *swConfig.ports()) {
      if (auto speedAndProfile =
              getSecondarySpeedAndProfile(*port.profileID())) {
        XLOG(INFO) << folly::sformat(
            "Changing speed and profile on port {:s} from speed={:s},profile={:s} to speed={:s},profile={:s}",
            port.name().ensure(),
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
    auto newCfg = AgentConfig(
        testConfig,
        apache::thrift::SimpleJSONSerializer::serialize<std::string>(
            testConfig));
    // Dump config to the file used by Agent Ensemble test to pick up warmboot
    // config
    newCfg.dumpConfig(getTestConfigPath());
  }

  void runSpeedChangeTest(cfg::PortSpeed fromSpeed, cfg::PortSpeed toSpeed) {
    auto setup = [this, fromSpeed, toSpeed]() {
      auto newConfig = createSpeedChangeConfig(fromSpeed, toSpeed);

      // Apply the new config. Call ApplyNewConfig so warmboot file is also
      // updated
      applyNewConfig(newConfig);

      EXPECT_NO_THROW(waitForAllCabledPorts(true));
      createL3DataplaneFlood();
    };
    auto verify = [this]() {
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

      if (getSw()->getBootType() == BootType::COLD_BOOT) {
        auto ecmpSizeInSw = getSingleVlanOrRoutedCabledPorts().size();
        XLOG(INFO) << "[DBG] ECMP size in sw: " << ecmpSizeInSw;
        auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
        facebook::fboss::utility::CIDRNetwork cidr;
        cidr.IPAddress() = "::";
        cidr.mask() = 0;

        WITH_RETRIES({
          EXPECT_EVENTUALLY_EQ(
              client->sync_getHwEcmpSize(cidr, 0, ecmpSizeInSw), ecmpSizeInSw);
        });
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  std::string originalConfigCopy;
};

void AgentEnsembleSpeedChangeTest::preInitSetup() {
  auto linkTestDir = FLAGS_volatile_state_dir + "/link_test";
  originalConfigCopy =
      linkTestDir + "/agent_speed_change_test_original_config.conf";
  utilCreateDir(linkTestDir);
  AgentEnsembleLinkTest::preInitSetup();
}

cfg::SwitchConfig AgentEnsembleSpeedChangeTest::initialConfig(
    const AgentEnsemble& ensemble) {
  // Coldboot iteration -
  //    SetUp -
  //      1. Make a copy of the original config. Existing copy is overridden
  //      2. The speeds and profiles are modified by this test in the original
  //      config.
  //    TearDown - Do nothing. Keep the modified config in the
  //    FLAGS_config path.
  // Warmboot iteration -
  //    SetUp - Do nothing. Agent loads up with the modified config (setup by
  //      coldboot iteration)
  //    TearDown - Copy is deleted if we are not asked to setup_for_warmboot
  //    again

  // TODO config is modified and stored in /agent_ensemble/agent.conf with the
  // new framework check if we need to do this moving forward
  if (ensemble.getBootType() == BootType::COLD_BOOT) {
    // Likely a coldboot iteration
    boost::filesystem::copy_file(
        FLAGS_config,
        originalConfigCopy,
        boost::filesystem::copy_option::overwrite_if_exists);
  }
  // Make sure the copy exists
  CHECK(boost::filesystem::exists(originalConfigCopy));
  return AgentEnsembleLinkTest::initialConfig(ensemble);
}

void AgentEnsembleSpeedChangeTest::TearDown() {
  if (!FLAGS_setup_for_warmboot) {
    // If this test wasn't setup for warmboot, revert back the original config
    // in FLAGS_config path. Also clean up the copy
    boost::filesystem::copy_file(
        originalConfigCopy,
        FLAGS_config,
        boost::filesystem::copy_option::overwrite_if_exists);
    CHECK(removeFile(originalConfigCopy));
  }
  AgentEnsembleLinkTest::TearDown();
}

// Returns a secondary speed if the platform supports it
std::optional<SpeedAndProfile>
AgentEnsembleSpeedChangeTest::getSecondarySpeedAndProfile(
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
TEST_F(AgentEnsembleSpeedChangeTest, secondarySpeed) {
  auto speedChangeSetup = [this]() {
    // Create a new config with secondary speed
    createSecondarySpeedConfig();
    auto cfg = *(AgentConfig::fromFile(getTestConfigPath())->thrift.sw());

    // Apply the new config
    applyNewConfig(cfg);

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
    auto ecmpSizeInSw = getSingleVlanOrRoutedCabledPorts().size();
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    facebook::fboss::utility::CIDRNetwork cidr;
    cidr.IPAddress() = "::";
    cidr.mask() = 0;
    EXPECT_EQ(client->sync_getHwEcmpSize(cidr, 0, ecmpSizeInSw), ecmpSizeInSw);
  };

  verifyAcrossWarmBoots(speedChangeSetup, speedChangeVerify);
}

/*
 * Coldboot Iteration
 *   - create a new config with changed speeds. Replace the original config
 * 1st Warmboot Iteration
 *   - Warm boot with the new config
 *   - Verify link health
 *   - Restore the original coldboot config
 * 2nd Warmboot Iteration
 *   - Warm boot with the original cold boot config
 *   - Verify link health
 */
TEST_F(AgentEnsembleSpeedChangeTest, speedChangeActivatedByWb) {
  auto speedChangeSetup = [this]() {
    // Create a new config with changed speeds
    createSecondarySpeedConfig();
    createL3DataplaneFlood();
  };
  auto speedChangeVerify = [this]() {
    /*
     * Verify the following on all cabled ports
     * 1. Link comes up at secondary speeds
     * 2. LLDP neighbor is discovered
     */
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    checkWithRetry(
        [this]() { return sendAndCheckReachabilityOnAllCabledPorts(); });

    if (getSw()->getBootType() == BootType::WARM_BOOT) {
      boost::filesystem::copy_file(
          originalConfigCopy,
          getTestConfigPath(),
          boost::filesystem::copy_option::overwrite_if_exists);
    }
  };

  verifyAcrossWarmBoots(speedChangeSetup, speedChangeVerify);
}

#define TEST_SPEED_CHANGE(from_speed, to_speed)                               \
  TEST_F(AgentEnsembleSpeedChangeTest, from_speed##To##to_speed) {            \
    runSpeedChangeTest(cfg::PortSpeed::from_speed, cfg::PortSpeed::to_speed); \
  }

TEST_SPEED_CHANGE(TWOHUNDREDG, HUNDREDG);

TEST_SPEED_CHANGE(HUNDREDG, TWOHUNDREDG);

TEST_SPEED_CHANGE(TWOHUNDREDG, FOURHUNDREDG);

TEST_SPEED_CHANGE(FOURHUNDREDG, TWOHUNDREDG);
