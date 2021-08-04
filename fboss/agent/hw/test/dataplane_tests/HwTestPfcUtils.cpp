/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss::utility {

std::vector<int> kLosslessPgs() {
  return {6, 7};
}

int kPgMinLimitCells() {
  return 19;
}
int kGlobalSharedBufferCells() {
  return 100000;
}
int kGlobalHeadroomBufferCells() {
  return 12432;
}
int kPgHeadroomLimitCells() {
  return 1156;
}
int kPgResumeLimitCells() {
  return 19;
}

void enablePfcConfig(cfg::SwitchConfig& cfg, const std::vector<PortID>& ports) {
  cfg::PortPfc pfc;
  pfc.tx_ref() = true;
  pfc.rx_ref() = true;
  pfc.portPgConfigName_ref() = "foo";

  for (const auto& portID : ports) {
    auto portCfg = utility::findCfgPort(cfg, portID);
    portCfg->pfc_ref() = pfc;
  }
}

void enablePfcMapsConfig(cfg::SwitchConfig& cfg) {
  // update pfc maps
  std::map<int16_t, int16_t> tc2PgId;
  std::map<int16_t, int16_t> pfcPri2PgId;
  std::map<int16_t, int16_t> pfcPri2QueueId;
  // program defaults
  // note PORT_PG_VALUE_MAX, PFC_PRIORITY_VALUE_MAX have same value
  for (auto i = 0; i <= cfg::switch_config_constants::PORT_PG_VALUE_MAX();
       i++) {
    tc2PgId.emplace(i, i);
    pfcPri2PgId.emplace(i, i);
    pfcPri2QueueId.emplace(i, i);
  }

  if (cfg.qosPolicies_ref().is_set() &&
      cfg.qosPolicies_ref()[0].qosMap_ref().has_value()) {
    cfg.qosPolicies_ref()[0].qosMap_ref()->trafficClassToPgId_ref() =
        std::move(tc2PgId);
    cfg.qosPolicies_ref()[0].qosMap_ref()->pfcPriorityToPgId_ref() =
        std::move(pfcPri2PgId);
    cfg.qosPolicies_ref()[0].qosMap_ref()->pfcPriorityToQueueId_ref() =
        std::move(pfcPri2QueueId);
  }
}

void enablePgConfigConfig(cfg::SwitchConfig& cfg, const int mmuCellBytes) {
  std::vector<cfg::PortPgConfig> portPgConfigs;
  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  for (const auto& pgId : kLosslessPgs()) {
    cfg::PortPgConfig pgConfig;
    CHECK_LE(pgId, cfg::switch_config_constants::PORT_PG_VALUE_MAX());
    pgConfig.id_ref() = pgId;
    pgConfig.bufferPoolName_ref() = "bufferNew";
    // provide atleast 1 cell worth of minLimit
    pgConfig.minLimitBytes_ref() = kPgMinLimitCells() * mmuCellBytes;
    pgConfig.headroomLimitBytes_ref() = kPgHeadroomLimitCells() * mmuCellBytes;
    pgConfig.scalingFactor_ref() = cfg::MMUScalingFactor::ONE;
    pgConfig.resumeOffsetBytes_ref() = kPgResumeLimitCells() * mmuCellBytes;
    portPgConfigs.emplace_back(pgConfig);
  }

  portPgConfigMap.insert({"foo", portPgConfigs});
  cfg.portPgConfigs_ref() = portPgConfigMap;
}

void enableBufferPoolConfig(cfg::SwitchConfig& cfg, const int mmuCellBytes) {
  // create buffer pool
  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
  cfg::BufferPoolConfig poolConfig;
  poolConfig.sharedBytes_ref() = kGlobalSharedBufferCells() * mmuCellBytes;
  poolConfig.headroomBytes_ref() = kGlobalHeadroomBufferCells() * mmuCellBytes;
  bufferPoolCfgMap.insert({"bufferNew", poolConfig});
  cfg.bufferPoolConfigs_ref() = bufferPoolCfgMap;
}

void addPfcConfig(
    cfg::SwitchConfig& cfg,
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports) {
  const int mmuCellBytes = hwSwitch->getPlatform()->getMMUCellBytes();
  enablePfcConfig(cfg, ports);
  enablePfcMapsConfig(cfg);
  enablePgConfigConfig(cfg, mmuCellBytes);
  enableBufferPoolConfig(cfg, mmuCellBytes);
}

}; // namespace facebook::fboss::utility
