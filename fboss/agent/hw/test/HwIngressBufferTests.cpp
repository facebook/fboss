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
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPfcUtils.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;

namespace {

// arbitrary values
constexpr int kPgResumeOffsetCells = 5;
constexpr int kPgMinLimitCells = 6;
constexpr int kPgHeadroomLimitCells = 2;
constexpr int kPoolHeadroomLimitCells = 10;
constexpr int kPoolSharedCells = 78;
/*
 * SDK has the expectation that pool_total_size - delta <= shared size,
 * where delta is the difference between old and new pool limits, if
 * not, its considered an error.
 */
constexpr int kPoolSharedSizeBytesHighValue = 113091;
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
  tmpCfg.sharedBytes() = kPoolSharedSizeBytesHighValue * mmuBytesInCell;
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

class HwIngressBufferTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
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

  cfg::SwitchConfig cfg_;
};

// Create PG config, associate with PFC config
// validate that SDK programming is as per the cfg
// Read back from HW (using SDK calls) and validate
TEST_F(HwIngressBufferTest, validateConfig) {
  auto setup = [=]() { setupHelper(); };

  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        true /*pfcEnable*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Create PG, Ingress pool config, associate with PFC config.
// Modify the ingress pool params only and ensure that it is
// getting re-programmed
TEST_F(HwIngressBufferTest, validateIngressPoolParamChange) {
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

// Create PG config, associate with PFC config. Modify the PG
// config params and ensure that its getting re-programmed.
TEST_F(HwIngressBufferTest, validatePGParamChange) {
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

// Validate the Pg's pfc mode bit, by default we have been enabling
// PFC on the port and hence on every PG. Force the port to have no
// PFC. Validate that Pg's pfc mode is False now.
TEST_F(HwIngressBufferTest, validatePgNoPfc) {
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

// Validate that if we program the global buffer with high values, we
// don't throw any errors programming. We saw this issue, when we try
// programming higher values and logic has been added to program what
// ever buffer value is lower than programmed first to workaround the
// sdk error.
TEST_F(HwIngressBufferTest, validateHighBufferValues) {
  auto setup = [&]() { setupHelperWithHighBufferValues(); };
  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        true /*pfcEnable*/);
  };
  verifyAcrossWarmBoots(setup, verify);
}

// Create PG config, associate with PFC config. Do not create headroom
// cfg, PGs should be in lossy mode now; validate that SDK programming
// is as per cfg.
TEST_F(HwIngressBufferTest, validateLossyMode) {
  auto setup = [&]() { setupHelper(false /* enable headroom */); };

  auto verify = [&]() {
    utility::checkSwHwPgCfgMatch(
        getHwSwitch(),
        getProgrammedState()->getPort(PortID(masterLogicalPortIds()[0])),
        true /*pfcEnable*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
