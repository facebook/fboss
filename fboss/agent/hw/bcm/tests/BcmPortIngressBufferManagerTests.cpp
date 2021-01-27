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

// arbitrary values
constexpr int kPgResumeOffsetCells = 5;
constexpr int kPgMinLimitCells = 6;
constexpr int kPgHeadroomLimitCells = 2;
constexpr int kPoolHeadroomLimitCells = 10;
constexpr int kPoolSharedCells = 12;
constexpr std::string_view kBufferPoolName = "fooBuffer";

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
    // use queueId value to assign different values for each param/queue
    pgConfig.headroomLimitBytes_ref() =
        (kPgHeadroomLimitCells + queueId + deltaValue) * mmuCellByes;
    pgConfig.minLimitBytes_ref() =
        (kPgMinLimitCells + queueId + deltaValue) * mmuCellByes;
    pgConfig.resumeOffsetBytes_ref() =
        (kPgResumeOffsetCells + queueId + deltaValue) * mmuCellByes;
    pgConfig.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
    pgConfig.bufferPoolName_ref() = kBufferPoolName;
    portPgConfigs.emplace_back(pgConfig);
  }
  return portPgConfigs;
}

cfg::BufferPoolConfig getBufferPoolConfig(int mmuCellByes, int deltaValue = 0) {
  // same as one in pg
  // std::string bufferName = "fooBuffer";
  cfg::BufferPoolConfig tmpCfg;
  tmpCfg.headroomBytes_ref() =
      (kPoolHeadroomLimitCells + deltaValue) * mmuCellByes;
  tmpCfg.sharedBytes_ref() = (kPoolSharedCells + deltaValue) * mmuCellByes;
  return tmpCfg;
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

    // setup bufferPool
    std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
    bufferPoolCfgMap.insert(make_pair(
        static_cast<std::string>(kBufferPoolName),
        getBufferPoolConfig(getPlatform()->getMMUCellBytes())));
    cfg.bufferPoolConfigs_ref() = std::move(bufferPoolCfgMap);
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

    const auto& defaultBufferPoolCfg = bcmPort->getDefaultIngressPoolSettings();
    auto bufferPoolCfgInHwPtr = bcmPort->getCurrentIngressPoolSettings();

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

    EXPECT_EQ(
        defaultBufferPoolCfg.getHeadroomBytes(),
        (*bufferPoolCfgInHwPtr).getHeadroomBytes());
    EXPECT_EQ(
        defaultBufferPoolCfg.getSharedBytes(),
        (*bufferPoolCfgInHwPtr).getSharedBytes());
  }

  // routine to validate if the SW and HW match for the PG cfg
  void checkSwHwPgCfgMatch() {
    PortPgConfigs portPgsHw;

    auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[0]));

    portPgsHw = bcmPort->getCurrentProgrammedPgSettings();
    const auto bufferPoolHwPtr = bcmPort->getCurrentIngressPoolSettings();

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
      const auto bufferPoolPtr = pgConfig->getBufferPoolConfig();
      EXPECT_EQ(
          (*bufferPoolPtr)->getSharedBytes(),
          (*bufferPoolHwPtr).getSharedBytes());
      EXPECT_EQ(
          (*bufferPoolPtr)->getHeadroomBytes(),
          (*bufferPoolHwPtr).getHeadroomBytes());
      i++;
    }
  }
  cfg::SwitchConfig cfg_;
};

// Create PG config, associate with PFC config
// validate that SDK programming is as per the cfg
// Read back from HW (using SDK calls) and validate
TEST_F(BcmPortIngressBufferManagerTest, validateConfig) {
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
TEST_F(BcmPortIngressBufferManagerTest, validateConfigReset) {
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

// Create PG, Ingress pool config, associate with PFC config
// Modify the ingress pool params onlyand ensure that its getting re-programmed
TEST_F(BcmPortIngressBufferManagerTest, validateIngressPoolParamChange) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support PFC";
    return;
  }

  auto setup = [=]() {
    setupHelper();
    // setup bufferPool
    std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
    bufferPoolCfgMap.insert(make_pair(
        static_cast<std::string>(kBufferPoolName),
        getBufferPoolConfig(getPlatform()->getMMUCellBytes(), 1)));
    cfg_.bufferPoolConfigs_ref() = bufferPoolCfgMap;
    // update one PG, and see ifs reflected in the HW
    applyNewConfig(cfg_);
  };

  auto verify = [&]() { checkSwHwPgCfgMatch(); };

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
