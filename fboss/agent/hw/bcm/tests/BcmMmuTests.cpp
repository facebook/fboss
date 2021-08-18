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
#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/types.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#if (!defined(IS_OPENNSA))
#include <soc/drv.h>
#endif

using namespace facebook::fboss;

namespace {
// based on discussion with BCM for TH3, MMU lossless
constexpr int kHighPriCpuQueueCells = 14;
} // unnamed namespace

namespace facebook::fboss {
class BcmMmuTests : public BcmTest {
 protected:
  void SetUp() override {
    FLAGS_mmu_lossless_mode = true;
    BcmTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  }

  void addCpuQueues(cfg::SwitchConfig& cfg) {
    int highPriorityCpuQueue = utility::getCoppHighPriQueueId(this->getAsic());
    const int mmuBytesPerCell = getPlatform()->getMMUCellBytes();
    const int reservedBytes = this->getAsic()->getDefaultReservedBytes(
        cfg::StreamType::MULTICAST, true);
    std::vector<cfg::PortQueue> cpuQueues;

    utility::addCpuQueueConfig(cfg, this->getAsic());
    for (auto cpuQueue : *cfg_.cpuQueues_ref()) {
      if (*cpuQueue.id_ref() == utility::kCoppLowPriQueueId ||
          *cpuQueue.id_ref() == utility::kCoppMidPriQueueId ||
          *cpuQueue.id_ref() == utility::kCoppDefaultPriQueueId) {
        cpuQueue.reservedBytes_ref() = reservedBytes;
      } else if (*cpuQueue.id_ref() == highPriorityCpuQueue) {
        cpuQueue.reservedBytes_ref() = kHighPriCpuQueueCells * mmuBytesPerCell;
      }
      cpuQueues.push_back(cpuQueue);
    }
    // update the cpu queues based on new values
    cfg.cpuQueues_ref() = cpuQueues;
  }

  // Basic config with 2 L3 interface config
  void setupBaseConfig() {
    cfg_ = initialConfig();
    // add cpu queues
    addCpuQueues(cfg_);
    applyNewConfig(cfg_);
  }

  void setupPfcConfig() {
    cfg_ = initialConfig();
    std::vector<PortID> cfgPorts = {
        masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
    utility::addPfcConfig(cfg_, getHwSwitch(), cfgPorts);
    applyNewConfig(cfg_);
  }

  // we are looking to generate 32 ports
  // 16 from each ITM. Explicitly Mark other ports
  // as downlinl and in DISABLED state as we
  // have. So 16 uplinks, 16 UP downlinks, rest all DISABLED
  // downlinks
  // There are 8 pipes and 4 pipes map to each ITM
  // Ensure we have 2 downlinks, 2 uplinks on each pipe to
  // get 32 ports
  void setupPortsForZionEx(
      cfg::SwitchConfig& cfg,
      std::vector<PortID>& uplinkPorts,
      std::vector<PortID>& downlinkPorts) {
    std::map<int, std::vector<PortID>> pipeToPortMap;
    // create map of pipe <-> ports
    for (const auto& mp : masterLogicalPortIds()) {
      auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(mp);
      auto pipe = bcmPort->getPipe();
      auto pipeToPortMapIter = pipeToPortMap.find(pipe);
      if (pipeToPortMapIter != pipeToPortMap.end()) {
        pipeToPortMapIter->second.emplace_back(mp);
      } else {
        pipeToPortMap[pipe] = {mp};
      }
    }

    // assign ports to uplink, downlink
    for (const auto& pipeToPortEntry : pipeToPortMap) {
      // for every pipe, pick first 2 uplinks, rest all downlinks
      int uplinkCount = 0;
      int downlinkCount = 0;
      for (const auto& port : pipeToPortEntry.second) {
        auto portCfg = utility::findCfgPort(cfg, port);
        if (uplinkCount < 2) {
          uplinkPorts.emplace_back(static_cast<PortID>(port));
          uplinkCount++;
        } else {
          downlinkPorts.emplace_back(static_cast<PortID>(port));
          if (downlinkCount >= 2) {
            portCfg->state_ref() = cfg::PortState::DISABLED;
          }
          downlinkCount++;
        }
      }
    }
  }

  void setupZionExConfig() {
    std::vector<PortID> uplinkPorts, downlinkPorts;
    cfg_ = initialConfig();
    setupPortsForZionEx(cfg_, uplinkPorts, downlinkPorts);
    utility::addUplinkDownlinkPfcConfig(
        cfg_, getHwSwitch(), uplinkPorts, downlinkPorts);
    // add cpu queues
    addCpuQueues(cfg_);
    // apply new config
    applyNewConfig(cfg_);
  }

 private:
  cfg::SwitchConfig cfg_;
};

// intent of this test is to validate that
// other than first 10 (maxConfiguredCpuQueues) CPU queues
// all other queues have 0 reserved bytes allocated
// this is achieved using the following bcm cfg knobs
// (1) buf.mqueue.guarantee.0 (2) mmu_config_override
TEST_F(BcmMmuTests, CpuQueueReservedBytes) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support MMU lossless mode/PFC";
    return;
  }
  auto setup = [=]() { setupBaseConfig(); };
  auto verify = [=]() {
    const int numCpuQueues = NUM_CPU_COSQ(getUnit());
    const int mmuBytesPerCell = getPlatform()->getMMUCellBytes();
    const int defaultCpuReservedBytes =
        this->getAsic()->getDefaultReservedBytes(
            cfg::StreamType::MULTICAST, true);
    const int maxConfiguredCpuQueues =
        getHwSwitch()->getControlPlane()->getCPUQueues();
    for (int i = 0; i < numCpuQueues; ++i) {
      int bytes = getHwSwitch()->getControlPlane()->getReservedBytes(
          static_cast<bcm_cos_queue_t>(i));
      if (i == utility::getCoppHighPriQueueId(this->getAsic())) {
        // this one is special
        EXPECT_EQ(kHighPriCpuQueueCells * mmuBytesPerCell, bytes);
      } else if (i < maxConfiguredCpuQueues) {
        EXPECT_EQ(defaultCpuReservedBytes, bytes);
      } else {
        EXPECT_EQ(0, bytes);
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

// validate that pgMin limits on every port are set to 0
// except the ones are expicitly programmed.
// this is done using buf.prigroup7.guarantee bcm cfg knob
TEST_F(BcmMmuTests, PgMinLimitBytes) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support MMU lossless mode/PFC";
    return;
  }
  auto setup = [&]() { setupPfcConfig(); };
  auto verify = [&]() {
    std::set<PortID> cfgPorts{
        masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
    const int mmuBytesPerCell = getPlatform()->getMMUCellBytes();
    for (const auto& portId : masterLogicalPortIds()) {
      std::array<int, cfg::switch_config_constants::PORT_PG_VALUE_MAX() + 1>
          expectedValues = {0};
      if (cfgPorts.find(portId) != cfgPorts.end()) {
        // we expect PGMin values ot be set only for the lossless PGs
        // on programmed ports only
        for (const auto pgId : utility::kLosslessPgs()) {
          EXPECT_LE(pgId, cfg::switch_config_constants::PORT_PG_VALUE_MAX());
          expectedValues[pgId] = utility::kPgMinLimitCells() * mmuBytesPerCell;
        }
      }
      auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(portId);
      for (int i = 0; i < cfg::switch_config_constants::PORT_PG_VALUE_MAX() + 1;
           ++i) {
        EXPECT_EQ(bcmPort->getPgMinLimitBytes(i), expectedValues[i]);
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

// intent of this test is to mimic zionex port map
// and ensure that production MMU cfg can be applied correctly
// to fboss/SDK
TEST_F(BcmMmuTests, ZionExMmuConfigMax) {
  if (!isSupported(HwAsic::Feature::PFC)) {
    XLOG(WARNING) << "Platform doesn't support MMU lossless mode/PFC";
    return;
  }

  auto setup = [&]() { setupZionExConfig(); };
  auto verify = [&]() {
    // validate that shared bytes is as expected or more on the
    // given platform
    // get first port in the list
    auto bcmPort =
        getHwSwitch()->getPortTable()->getBcmPort(masterLogicalPortIds()[0]);
    // get first pg
    auto pgId = utility::kLosslessPgs()[0];
    const int mmuBytesPerCell = getPlatform()->getMMUCellBytes();
    EXPECT_GE(
        bcmPort->getIngressSharedBytes(pgId),
        utility::kGlobalSharedBufferCells(getHwSwitch()) * mmuBytesPerCell);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
