/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmCosQueueFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortIngressBufferManager.h"
//#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/types.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

using namespace facebook::fboss;

namespace {

constexpr int kResumeOffsetCells = 5;
constexpr int kMinLimitCells = 6;
constexpr int kHeadroomLimitCells = 2;

// util to construct per PG params
// pass in number of queues and use queueId, deltaValue to differ
// PG params
std::vector<cfg::PortPgConfig> getPortPgConfig(
    int mmuCellByes,
    const std::vector<int>& queues,
    int deltaValue = 0) {
  std::vector<cfg::PortPgConfig> portPgConfigs;

  for (const auto queueId : queues) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id_ref() = queueId;
    pgConfig.headroomLimitBytes_ref() =
        (kHeadroomLimitCells + queueId + deltaValue) * mmuCellByes;
    pgConfig.minLimitBytes_ref() =
        (kMinLimitCells + queueId + deltaValue) * mmuCellByes;
    pgConfig.resumeOffsetBytes_ref() =
        (kResumeOffsetCells + queueId + deltaValue) * mmuCellByes;
    pgConfig.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
    portPgConfigs.emplace_back(pgConfig);
  }
  return portPgConfigs;
}

} // unnamed namespace

namespace facebook::fboss {

class BcmPortIngressBufferManagerTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }

  void setupHelper() {
    auto cfg = initialConfig();
    auto portCfg = utility::findCfgPort(cfg, masterLogicalPortIds()[0]);

    // setup pfc
    cfg::PortPfc pfc;
    pfc.portPgConfigName_ref() = "foo";
    portCfg->pfc_ref() = pfc;

    // setup pgConfig
    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
    portPgConfigMap["foo"] =
        getPortPgConfig(getPlatform()->getMMUCellBytes(), {0, 1});
    cfg.portPgConfigs_ref() = portPgConfigMap;
    applyNewConfig(cfg);
    cfg_ = cfg;
  }

  // routine to validate that default cfg matches HW
  void checkSwDefaultHwPgCfgMatch() {
    auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[0]));
    auto queues = bcmPort->getCurrentQueueSettings();
    auto portPgsProgrammedInHw = bcmPort->getCurrentProgrammedPgSettings();
    auto portPgsInHw = bcmPort->getCurrentPgSettings();
    const auto& pgConfig = bcmPort->getDefaultPgSettings();
    const auto& pgList = bcmPort->getIngressBufferManager()->getPgIdListInHw();

    EXPECT_EQ(pgList.size(), 0);
    EXPECT_EQ(
        portPgsInHw.size(),
        cfg::switch_config_constants::PORT_PG_VALUE_MAX() + 1);
    EXPECT_EQ(portPgsProgrammedInHw.size(), 0);

    // TODO: come back to enahnce this one to read for all PGs
    EXPECT_EQ(pgConfig.getScalingFactor(), portPgsInHw[0]->getScalingFactor());
    EXPECT_EQ(
        pgConfig.getResumeOffsetBytes().value(),
        portPgsInHw[0]->getResumeOffsetBytes().value());
    EXPECT_EQ(pgConfig.getMinLimitBytes(), portPgsInHw[0]->getMinLimitBytes());
    EXPECT_EQ(
        pgConfig.getHeadroomLimitBytes(),
        portPgsInHw[0]->getHeadroomLimitBytes());
  }

  // routine to validate if the SW and HW match for the PG cfg
  void checkSwHwPgCfgMatch() {
    PortPgConfigs portPgsHw;

    auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[0]));

    portPgsHw = bcmPort->getCurrentProgrammedPgSettings();
    auto swPort =
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0]));
    auto swPgConfig = swPort->getPortPgConfigs();

    EXPECT_EQ(swPort->getPortPgConfigs().value().size(), portPgsHw.size());

    int i = 0;
    // both vectors are sorted to start with lowest pg id
    for (const auto& pgConfig : *swPgConfig) {
      EXPECT_EQ(pgConfig->getID(), portPgsHw[i]->getID());
      EXPECT_EQ(pgConfig->getScalingFactor(), portPgsHw[i]->getScalingFactor());
      EXPECT_EQ(
          pgConfig->getResumeOffsetBytes().value(),
          portPgsHw[i]->getResumeOffsetBytes().value());
      EXPECT_EQ(pgConfig->getMinLimitBytes(), portPgsHw[i]->getMinLimitBytes());
      EXPECT_EQ(
          pgConfig->getHeadroomLimitBytes(),
          portPgsHw[i]->getHeadroomLimitBytes());
      i++;
    }
  }
  cfg::SwitchConfig cfg_;
};

// Create PG config, associate with PFC config
// validate that SDK programming is as per the cfg
// Read back from HW (using SDK calls) and validate
TEST_F(BcmPortIngressBufferManagerTest, validatePGConfig) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support PFC";
    return;
  }

  auto setup = [=]() { setupHelper(); };

  auto verify = [&]() { checkSwHwPgCfgMatch(); };

  verifyAcrossWarmBoots(setup, verify);
}

// Create PG config, associate with PFC config and reset it
// Ensure that its getting reset to the defaults in HW as expected
TEST_F(BcmPortIngressBufferManagerTest, validatePGConfigReset) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support PFC";
    return;
  }

  auto setup = [=]() {
    setupHelper();
    // reset PG config
    auto portCfg = utility::findCfgPort(cfg_, masterLogicalPortIds()[0]);
    portCfg->pfc_ref().reset();
    cfg_.portPgConfigs_ref().reset();
    applyNewConfig(cfg_);
  };

  auto verify = [&]() { checkSwDefaultHwPgCfgMatch(); };

  verifyAcrossWarmBoots(setup, verify);
}

// Create PG config, associate with PFC config
// Modify the PG config params and ensure that its getting re-programmed
TEST_F(BcmPortIngressBufferManagerTest, validatePGParamChange) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support PFC";
    return;
  }

  auto setup = [=]() {
    setupHelper();
    // update one PG, and see ifs reflected in the HW
    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
    portPgConfigMap["foo"] =
        getPortPgConfig(getPlatform()->getMMUCellBytes(), {0, 1}, 1);
    cfg_.portPgConfigs_ref() = portPgConfigMap;
    applyNewConfig(cfg_);
  };

  auto verify = [&]() { checkSwHwPgCfgMatch(); };

  verifyAcrossWarmBoots(setup, verify);
}

// Create PG config, associate with PFC config
// Modify the PG queue config params and ensure that its getting re-programmed
TEST_F(BcmPortIngressBufferManagerTest, validatePGQueueChanges) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support PFC";
    return;
  }

  auto setup = [=]() {
    setupHelper();
    // update one PG, and see ifs reflected in the HW
    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
    portPgConfigMap["foo"] =
        getPortPgConfig(getPlatform()->getMMUCellBytes(), {1});
    cfg_.portPgConfigs_ref() = portPgConfigMap;
    applyNewConfig(cfg_);
  };

  auto verify = [&]() {
    checkSwHwPgCfgMatch();
    // retrive the PG list thats explicitly programmed
    auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[0]));
    const auto& pgList = bcmPort->getIngressBufferManager()->getPgIdListInHw();
    PgIdSet pgIdSetExpected = {1};
    EXPECT_EQ(pgList, pgIdSetExpected);
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
