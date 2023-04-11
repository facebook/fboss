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
#include "fboss/agent/hw/test/HwTestPfcUtils.h"
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
    int mmuCellBytes,
    const std::vector<int>& queues,
    int deltaValue = 0,
    const bool enableHeadroom = true) {
  std::vector<cfg::PortPgConfig> portPgConfigs;

  for (const auto queueId : queues) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = queueId;
    // use queueId value to assign different values for each param/queue
    if (enableHeadroom) {
      pgConfig.headroomLimitBytes() =
          (kPgHeadroomLimitCells + queueId + deltaValue) * mmuCellBytes;
    }
    pgConfig.minLimitBytes() =
        (kPgMinLimitCells + queueId + deltaValue) * mmuCellBytes;
    pgConfig.resumeOffsetBytes() =
        (kPgResumeOffsetCells + queueId + deltaValue) * mmuCellBytes;
    pgConfig.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
    pgConfig.bufferPoolName() = kBufferPoolName;
    portPgConfigs.emplace_back(pgConfig);
  }
  return portPgConfigs;
}

cfg::BufferPoolConfig getBufferPoolHighDefaultConfig(int mmuBytesInCell) {
  cfg::BufferPoolConfig tmpCfg;
  tmpCfg.headroomBytes() = 12432 * mmuBytesInCell;
  tmpCfg.sharedBytes() = 119044 * mmuBytesInCell;
  return tmpCfg;
}

cfg::BufferPoolConfig getBufferPoolConfig(
    int mmuBytesInCell,
    int deltaValue = 0) {
  // same as one in pg
  // std::string bufferName = "fooBuffer";
  cfg::BufferPoolConfig tmpCfg;
  tmpCfg.headroomBytes() =
      (kPoolHeadroomLimitCells + deltaValue) * mmuBytesInCell;
  tmpCfg.sharedBytes() = (kPoolSharedCells + deltaValue) * mmuBytesInCell;
  return tmpCfg;
}

} // unnamed namespace

namespace facebook::fboss {

class BcmPortIngressBufferManagerTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }

  void setupGlobalBuffer(cfg::SwitchConfig& cfg, bool useLargeHwValues) {
    std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
    cfg::BufferPoolConfig bufferPoolConfig;
    if (useLargeHwValues) {
      bufferPoolConfig = getBufferPoolHighDefaultConfig(
          getPlatform()->getAsic()->getPacketBufferUnitSize());
    } else {
      bufferPoolConfig = getBufferPoolConfig(
          getPlatform()->getAsic()->getPacketBufferUnitSize());
    }

    bufferPoolCfgMap.insert(
        make_pair(static_cast<std::string>(kBufferPoolName), bufferPoolConfig));
    cfg.bufferPoolConfigs() = std::move(bufferPoolCfgMap);
  }

  void setupPgBuffers(cfg::SwitchConfig& cfg, const bool enableHeadroom) {
    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
    portPgConfigMap["foo"] = getPortPgConfig(
        getPlatform()->getAsic()->getPacketBufferUnitSize(),
        {0, 1},
        0 /* delta value */,
        enableHeadroom);
    cfg.portPgConfigs() = portPgConfigMap;
  }

  void
  setupPfc(cfg::SwitchConfig& cfg, const PortID& portId, const bool pfcEnable) {
    auto portCfg = utility::findCfgPort(cfg, portId);
    // setup pfc
    cfg::PortPfc pfc;
    pfc.portPgConfigName() = "foo";
    pfc.tx() = pfcEnable;
    portCfg->pfc() = pfc;
  }

  void setupHelper(
      bool enableHeadroom = true,
      bool pfcEnable = true,
      bool enableHighBufferValues = false) {
    auto cfg = initialConfig();

    // setup PFC
    setupPfc(cfg, masterLogicalPortIds()[0], pfcEnable);
    // setup pgConfig
    setupPgBuffers(cfg, enableHeadroom);
    // setup bufferPool
    setupGlobalBuffer(cfg, enableHighBufferValues);
    applyNewConfig(cfg);
    cfg_ = cfg;
  }

  void setupHelperWithHighBufferValues() {
    setupHelper(true, true, true /* enable high buffer defaults */);
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

    EXPECT_EQ(bcmPort->getProgrammedPgLosslessMode(0), 0);
    EXPECT_EQ(bcmPort->getProgrammedPfcStatusInPg(0), 0);
  }

  cfg::SwitchConfig cfg_;
};

// Create PG config, associate with PFC config and reset it
// Ensure that its getting reset to the defaults in HW as expected
TEST_F(BcmPortIngressBufferManagerTest, validateConfigReset) {
  auto setup = [&]() {
    setupHelper();
    // reset PG config
    auto portCfg = utility::findCfgPort(cfg_, masterLogicalPortIds()[0]);
    portCfg->pfc().reset();
    cfg_.portPgConfigs().reset();
    applyNewConfig(cfg_);
  };

  auto verify = [&]() { checkSwDefaultHwPgCfgMatch(); };

  verifyAcrossWarmBoots(setup, verify);
}

// Create PG, Ingress pool config, associate with PFC config
// Modify the ingress pool params onlyand ensure that its getting re-programmed
TEST_F(BcmPortIngressBufferManagerTest, validateIngressPoolParamChange) {
  auto setup = [&]() {
    setupHelper();
    // setup bufferPool
    std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
    bufferPoolCfgMap.insert(make_pair(
        static_cast<std::string>(kBufferPoolName),
        getBufferPoolConfig(
            getPlatform()->getAsic()->getPacketBufferUnitSize(), 1)));
    cfg_.bufferPoolConfigs() = bufferPoolCfgMap;
    // update one PG, and see ifs reflected in the HW
    applyNewConfig(cfg_);
  };

  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        true /*pfcEnable*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Create PG config, associate with PFC config
// Modify the PG config params and ensure that its getting re-programmed
TEST_F(BcmPortIngressBufferManagerTest, validatePGParamChange) {
  auto setup = [&]() {
    setupHelper();
    // update one PG, and see ifs reflected in the HW
    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
    portPgConfigMap["foo"] = getPortPgConfig(
        getPlatform()->getAsic()->getPacketBufferUnitSize(), {0, 1}, 1);
    cfg_.portPgConfigs() = portPgConfigMap;
    applyNewConfig(cfg_);
  };

  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        true /*pfcEnable*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Create PG config, associate with PFC config
// Modify the PG queue config params and ensure that its getting re-programmed
TEST_F(BcmPortIngressBufferManagerTest, validatePGQueueChanges) {
  auto setup = [&]() {
    setupHelper();
    // update one PG, and see ifs reflected in the HW
    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
    portPgConfigMap["foo"] = getPortPgConfig(
        getPlatform()->getAsic()->getPacketBufferUnitSize(), {1});
    cfg_.portPgConfigs() = portPgConfigMap;
    applyNewConfig(cfg_);
  };

  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        true /*pfcEnable*/);
    // retrive the PG list thats explicitly programmed
    auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[0]));
    const auto& pgList = bcmPort->getIngressBufferManager()->getPgIdListInHw();
    PgIdSet pgIdSetExpected = {1};
    EXPECT_EQ(pgList, pgIdSetExpected);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Create PG config, associate with PFC config
// do not create the headroom cfg, PGs should be in lossy mode now
// validate that SDK programming is as per the cfg
TEST_F(BcmPortIngressBufferManagerTest, validateLossyMode) {
  auto setup = [&]() { setupHelper(false /* enable headroom */); };

  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        true /*pfcEnable*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// validate the Pg's pfc mode bit
// by default we have been enabling PFC on the port and hence
// on every PG. Force the port to have no PFC
// Validate that Pg's pfc mode is False now
TEST_F(BcmPortIngressBufferManagerTest, validatePgNoPfc) {
  auto setup = [&]() {
    setupHelper(true /* enable headroom */, false /* pfc */);
  };
  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        false /*pfcEnable*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// validate that if we program the global buffer with high values
// we don't throw any errors programming
// we saw this issue, when we try programming higher values and logic
// has been added to program whaever buffer value is lower than
// programmed first to workaround the sdk error
TEST_F(BcmPortIngressBufferManagerTest, validateHighBufferValues) {
  auto setup = [&]() { setupHelperWithHighBufferValues(); };
  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        true /*pfcEnable*/);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
