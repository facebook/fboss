// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/lib/phy/PhyUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/PhyCapabilities.h"

namespace facebook::fboss {
template <cfg::PortProfileID Profile>
class HwPortProfileTest : public HwTest {
 protected:
  void SetUp() override {
    utility::enableTransceiverProgramming(true);
    HwTest::SetUp();
  }

  cfg::SwitchConfig initialConfig(const std::vector<PortID>& ports) const {
    auto lbMode = getPlatform()->getAsic()->desiredLoopbackModes();
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch()->getPlatform()->getPlatformMapping(),
        getHwSwitch()->getPlatform()->getAsic(),
        ports[0],
        ports[1],
        getHwSwitch()->getPlatform()->supportsAddRemovePort(),
        lbMode);
  }

  void verifyPlatformMapping(PortID port) {
    auto pPort = getPlatform()->getPlatformPort(port);
    EXPECT_EQ(
        pPort->getPlatformPortEntry(),
        getPlatform()->getPlatformMapping()->getPlatformPort(port));

    auto swPort = getProgrammedState()->getPort(port);
    auto matcher = PlatformPortProfileConfigMatcher(
        swPort->getProfileID(), swPort->getID());

    if (auto portProfileCfg =
            getPlatform()->getPlatformMapping()->getPortProfileConfig(
                matcher)) {
      EXPECT_EQ(
          pPort->getPortProfileConfigFromCache(swPort->getProfileID()),
          *portProfileCfg);
    }
    EXPECT_EQ(
        pPort->getPortPinConfigs(swPort->getProfileID()),
        getPlatform()->getPlatformMapping()->getPortXphyPinConfig(matcher));
    EXPECT_EQ(
        pPort->getPortDataplaneChips(swPort->getProfileID()),
        getPlatform()->getPlatformMapping()->getPortDataplaneChips(matcher));
  }

  void verifyPort(PortID portID) {
    auto platformPort = getPlatform()->getPlatformPort(portID);
    EXPECT_EQ(portID, platformPort->getPortID());
    auto port = getProgrammedState()->getPorts()->getNodeIf(portID);
    // verify interface mode
    utility::verifyInterfaceMode(
        port->getID(),
        port->getProfileID(),
        getPlatform(),
        port->getProfileConfig());
    // verify tx settings
    utility::verifyTxSettting(
        port->getID(),
        port->getProfileID(),
        getPlatform(),
        port->getPinConfigs());
    // verify rx settings
    utility::verifyRxSettting(
        port->getID(),
        port->getProfileID(),
        getPlatform(),
        port->getPinConfigs());
    // (TODO): verify speed
    utility::verifyFec(
        port->getID(),
        port->getProfileID(),
        getPlatform(),
        port->getProfileConfig());
    // (TODO): verify lane count (for sai)

    verifyPlatformMapping(port->getID());
  }

  // Verifies that we can read various PHY diagnostics but not the correctness
  // of these diagnostics
  void verifyPhyInfo(phy::PhyInfo& phyInfo, PortID portID) {
    if (getPlatform()->getAsic()->getAsicType() ==
            cfg::AsicType::ASIC_TYPE_TRIDENT2 ||
        getPlatform()->getAsic()->getAsicType() ==
            cfg::AsicType::ASIC_TYPE_FAKE ||
        getPlatform()->getAsic()->getAsicType() ==
            cfg::AsicType::ASIC_TYPE_MOCK) {
      return;
    }
    auto port = getProgrammedState()->getPorts()->getNodeIf(portID);
    auto expectedNumPmdLanes = port->getPinConfigs().size();

    // Start with the expectation that PMD diagnostics are available if
    // supported by SDK and then exclude certain cases below
    bool expectPmdSignalDetect = rxSignalDetectSupportedInSdk();
    bool expectPmdCdrLock = rxLockStatusSupportedInSdk();
    if (getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_TOMAHAWK) {
      // TH will never support these diagnostics with native or SAI SDKs
      expectPmdSignalDetect = false;
      expectPmdCdrLock = false;
    }

    bool expectPcsRxLinkStatus = pcsRxLinkStatusSupportedInSdk();
    bool expectFecAMLock = fecAlignmentLockSupportedInSdk();

    auto serializedSnapshot =
        apache::thrift::SimpleJSONSerializer::serialize<std::string>(phyInfo);
    XLOG(DBG3) << "Snapshot for port " << portID << " = " << serializedSnapshot;

    auto& state = *phyInfo.state();
    // Verify PhyState fields
    EXPECT_EQ(state.phyChip()->type(), phy::DataPlanePhyChipType::IPHY);
    EXPECT_TRUE(state.linkState().has_value());
    EXPECT_FALSE(state.system().has_value());
    EXPECT_TRUE(*state.timeCollected());
    EXPECT_EQ(state.speed(), port->getSpeed());
    EXPECT_EQ(state.name(), port->getName());

    auto& lineState = *state.line();
    EXPECT_EQ(lineState.side(), phy::Side::LINE);
#if 0
    // TODO: Fix for BcmPort and then enable this check
    EXPECT_TRUE(*lineState.medium() != TransmitterTechnology::UNKNOWN);
#endif
    auto& pmdState = *lineState.pmd();

    // Verify PmdState fields.
    if (expectPmdCdrLock || expectPmdSignalDetect) {
      EXPECT_EQ(pmdState.lanes()->size(), expectedNumPmdLanes);
    }
    std::unordered_set<int> laneIds;
    for (auto lane : *pmdState.lanes()) {
      auto laneState = lane.second;
      // Unique lanes
      EXPECT_TRUE(laneIds.find(*laneState.lane()) == laneIds.end());
      laneIds.insert(*laneState.lane());
      if (expectPmdSignalDetect) {
        EXPECT_TRUE(laneState.signalDetectLive().has_value());
        EXPECT_TRUE(laneState.signalDetectChanged().has_value());
      }
      if (expectPmdCdrLock) {
        EXPECT_TRUE(laneState.cdrLockLive().has_value());
        EXPECT_TRUE(laneState.cdrLockChanged().has_value());
      }
    }

    // Verify PhyStats
    auto& lineStats = *phyInfo.stats()->line();

    // Verify PmdStats
    if (expectPmdCdrLock || expectPmdSignalDetect) {
      EXPECT_EQ(lineStats.pmd()->lanes()->size(), expectedNumPmdLanes);
    }
    auto& pmdStats = *lineStats.pmd();
    laneIds.clear();
    for (auto lane : *pmdStats.lanes()) {
      auto laneStats = lane.second;
      // Unique lanes
      EXPECT_TRUE(laneIds.find(*laneStats.lane()) == laneIds.end());
      laneIds.insert(*laneStats.lane());
      if (expectPmdSignalDetect) {
        EXPECT_TRUE(laneStats.signalDetectChangedCount().has_value());
      }
      if (expectPmdCdrLock) {
        EXPECT_TRUE(laneStats.cdrLockChangedCount().has_value());
      }
    }

    if (expectPcsRxLinkStatus) {
      ASSERT_TRUE(lineState.pcs().has_value());
      ASSERT_TRUE(lineState.pcs()->pcsRxStatusLive().has_value());
      ASSERT_TRUE(lineState.pcs()->pcsRxStatusLatched().has_value());
    }

    // Verify RsFEC counters if applicable
    auto isRsFec =
        utility::isReedSolomonFec(getHwSwitch()->getPortFECMode(portID));
    if (isRsFec) {
      ASSERT_TRUE(lineStats.pcs().has_value());
      ASSERT_TRUE(lineStats.pcs()->rsFec().has_value());

      if (expectFecAMLock) {
        ASSERT_TRUE(lineState.pcs().has_value());
        ASSERT_TRUE(lineState.pcs()->rsFecState().has_value());
        ASSERT_TRUE(
            lineState.pcs()->rsFecState()->lanes()->size() ==
            utility::reedSolomonFecLanes(port->getSpeed()));
        for (auto fecLane :
             *lineState.pcs().ensure().rsFecState().ensure().lanes()) {
          ASSERT_TRUE(fecLane.second.fecAlignmentLockLive().has_value());
          ASSERT_TRUE(fecLane.second.fecAlignmentLockChanged().has_value());
        }
      }
    }
  }

  void runTest() {
    const auto& availablePorts = findAvailablePorts();
    std::map<PortID, phy::PhyInfo> allPhyInfo;
    if (availablePorts.size() < 2) {
// profile is not supported.
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    utility::enableSixtapProgramming();
    auto setup = [=, this]() {
      auto config = initialConfig(availablePorts);
      for (auto port : {availablePorts[0], availablePorts[1]}) {
        auto hwSwitch = getHwSwitch();
        utility::configurePortProfile(
            hwSwitch->getPlatform()->getPlatformMapping(),
            hwSwitch->getPlatform()->supportsAddRemovePort(),
            config,
            Profile,
            getAllPortsInGroup(port),
            port);
      }
      applyNewConfig(config);
    };
    auto verify = [this, &availablePorts, &allPhyInfo]() {
      getHwSwitch()->updateAllPhyInfo();
      allPhyInfo = getHwSwitch()->getAllPhyInfo();
      for (auto portID : {availablePorts[0], availablePorts[1]}) {
        verifyPort(portID);
        ASSERT_TRUE(allPhyInfo.find(portID) != allPhyInfo.end());
        verifyPhyInfo(allPhyInfo[portID], portID);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  std::optional<TransceiverInfo> overrideTransceiverInfo() const override {
    return utility::getTransceiverInfo(Profile);
  }

 private:
  std::vector<PortID> findAvailablePorts() {
    std::vector<PortID> availablePorts;
    auto& platformPorts = getPlatform()->getPlatformPorts();
    for (auto port : masterLogicalPortIds()) {
      auto iter = platformPorts.find(port);
      EXPECT_TRUE(iter != platformPorts.end());
      if (!getPlatform()->getPortProfileConfig(
              PlatformPortProfileConfigMatcher(Profile, port))) {
        continue;
      }
      if (iter->second.supportedProfiles()->find(Profile) ==
          iter->second.supportedProfiles()->end()) {
        continue;
      }
      availablePorts.push_back(port);
    }
    return availablePorts;
  }
};

#define TEST_PROFILE(PROFILE)                                     \
  struct HwTest_##PROFILE                                         \
      : public HwPortProfileTest<cfg::PortProfileID::PROFILE> {}; \
  TEST_F(HwTest_##PROFILE, TestProfile) {                         \
    runTest();                                                    \
  }

TEST_PROFILE(PROFILE_10G_1_NRZ_NOFEC_COPPER)

TEST_PROFILE(PROFILE_10G_1_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_25G_1_NRZ_NOFEC_COPPER)

TEST_PROFILE(PROFILE_25G_1_NRZ_CL74_COPPER)

TEST_PROFILE(PROFILE_25G_1_NRZ_RS528_COPPER)

TEST_PROFILE(PROFILE_40G_4_NRZ_NOFEC_COPPER)

TEST_PROFILE(PROFILE_40G_4_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_50G_2_NRZ_NOFEC_COPPER)

TEST_PROFILE(PROFILE_50G_2_NRZ_CL74_COPPER)

TEST_PROFILE(PROFILE_50G_2_NRZ_RS528_COPPER)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528_COPPER)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528_OPTICAL)

TEST_PROFILE(PROFILE_100G_4_NRZ_CL91_COPPER)

TEST_PROFILE(PROFILE_100G_4_NRZ_CL91_OPTICAL)

TEST_PROFILE(PROFILE_25G_1_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_50G_2_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_100G_4_NRZ_NOFEC_COPPER)

// TODO: investigate and fix failures
// TEST_PROFILE(PROFILE_20G_2_NRZ_NOFEC_COPPER)
// TEST_PROFILE(PROFILE_20G_2_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N_COPPER)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_400G_8_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528)

TEST_PROFILE(PROFILE_40G_4_NRZ_NOFEC)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N)

TEST_PROFILE(PROFILE_100G_4_NRZ_CL91_COPPER_RACK_YV3_T1)

TEST_PROFILE(PROFILE_25G_1_NRZ_NOFEC_COPPER_RACK_YV3_T1)

TEST_PROFILE(PROFILE_400G_8_PAM4_RS544X2N_COPPER)

TEST_PROFILE(PROFILE_400G_4_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_800G_8_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_100G_2_PAM4_RS544X2N_COPPER)

} // namespace facebook::fboss
