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

const std::vector<int> kLosslessPgs{0, 7};
constexpr int kBytesInMMUCell = 254;
constexpr int kGlobalSharedBufferCells = 100000 * kBytesInMMUCell;
constexpr int kGlobalHeadroomBufferCells = 12432 * kBytesInMMUCell;
constexpr int kPgMinLimitCells = 19 * kBytesInMMUCell;
constexpr int kPgHeadroomLimitCells = 1156 * kBytesInMMUCell;
constexpr int kPgResumeLimitCells = 19 * kBytesInMMUCell;

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

void enablePgConfigConfig(cfg::SwitchConfig& cfg) {
  std::vector<cfg::PortPgConfig> portPgConfigs;
  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  for (const auto& pgId : kLosslessPgs) {
    cfg::PortPgConfig pgConfig;
    CHECK_LE(pgId, cfg::switch_config_constants::PORT_PG_VALUE_MAX());
    pgConfig.id_ref() = pgId;
    pgConfig.bufferPoolName_ref() = "bufferNew";
    // provide atleast 1 cell worth of minLimit
    pgConfig.minLimitBytes_ref() = kPgMinLimitCells;
    pgConfig.headroomLimitBytes_ref() = kPgHeadroomLimitCells;
    pgConfig.scalingFactor_ref() = cfg::MMUScalingFactor::ONE;
    pgConfig.resumeOffsetBytes_ref() = kPgResumeLimitCells;
    portPgConfigs.emplace_back(pgConfig);
  }

  portPgConfigMap.insert({"foo", portPgConfigs});
  cfg.portPgConfigs_ref() = portPgConfigMap;
}

void enableBufferPoolConfig(cfg::SwitchConfig& cfg) {
  // create buffer pool
  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
  cfg::BufferPoolConfig poolConfig;
  poolConfig.sharedBytes_ref() = kGlobalSharedBufferCells;
  poolConfig.headroomBytes_ref() = kGlobalHeadroomBufferCells;
  bufferPoolCfgMap.insert({"bufferNew", poolConfig});
  cfg.bufferPoolConfigs_ref() = bufferPoolCfgMap;
}

void addPfcConfig(cfg::SwitchConfig& cfg, const std::vector<PortID>& ports) {
  enablePfcConfig(cfg, ports);
  enablePfcMapsConfig(cfg);
  enablePgConfigConfig(cfg);
  enableBufferPoolConfig(cfg);
}

}; // namespace facebook::fboss::utility
