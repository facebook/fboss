// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

namespace {

static const std::vector<int> kLossyPgIds{0};

void setupQosMapForPfc(
    cfg::QosMap& qosMap,
    const std::map<int, int>& tc2PgOverride = {},
    const std::map<int, int>& pfcPri2PgIdOverride = {}) {
  // update pfc maps
  std::map<int16_t, int16_t> tc2PgId;
  std::map<int16_t, int16_t> tc2QueueId;
  std::map<int16_t, int16_t> pfcPri2PgId;
  std::map<int16_t, int16_t> pfcPri2QueueId;
  // program defaults
  for (auto i = 0; i < 8; i++) {
    tc2PgId.emplace(i, i);
    // Jericho3 cpu/recycle port only has 2 egress queues. Tomahawk has more
    // queues, but we stick to the lowest common denominator here.
    // See https://fburl.com/gdoc/nyyg1cve and https://fburl.com/code/mhdeuiky
    tc2QueueId.emplace(i, i < 7 ? 0 : 1);
    pfcPri2PgId.emplace(i, i);
    pfcPri2QueueId.emplace(i, i);
  }

  for (auto& tc2Pg : tc2PgOverride) {
    tc2PgId[tc2Pg.first] = tc2Pg.second;
  }
  for (auto& tmp : pfcPri2PgIdOverride) {
    pfcPri2PgId[tmp.first] = tmp.second;
  }

  qosMap.dscpMaps()->resize(8);
  for (auto i = 0; i < 8; i++) {
    qosMap.dscpMaps()[i].internalTrafficClass() = i;
    for (auto j = 0; j < 8; j++) {
      qosMap.dscpMaps()[i].fromDscpToTrafficClass()->push_back(8 * i + j);
    }
  }
  qosMap.trafficClassToPgId() = std::move(tc2PgId);
  qosMap.trafficClassToQueueId() = std::move(tc2QueueId);
  qosMap.pfcPriorityToPgId() = std::move(pfcPri2PgId);
  qosMap.pfcPriorityToQueueId() = std::move(pfcPri2QueueId);
}

void setupPfc(cfg::SwitchConfig& cfg, const std::vector<PortID>& ports) {
  cfg::PortPfc pfc;
  pfc.tx() = true;
  pfc.rx() = true;
  pfc.portPgConfigName() = "foo";

  cfg::QosMap qosMap;
  // setup qos map with pfc structs
  setupQosMapForPfc(qosMap);

  // setup qosPolicy
  cfg.qosPolicies()->resize(1);
  cfg.qosPolicies()[0].name() = "qp";
  cfg.qosPolicies()[0].qosMap() = std::move(qosMap);
  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  dataPlaneTrafficPolicy.defaultQosPolicy() = "qp";
  cfg.dataPlaneTrafficPolicy() = std::move(dataPlaneTrafficPolicy);

  for (const auto& portID : ports) {
    auto portCfg = std::find_if(
        cfg.ports()->begin(), cfg.ports()->end(), [&portID](auto& port) {
          return PortID(*port.logicalID()) == portID;
        });
    portCfg->pfc() = pfc;
  }
}

void setupBufferPoolConfig(
    std::map<std::string, cfg::BufferPoolConfig>& bufferPoolCfgMap,
    int globalSharedBytes,
    int globalHeadroomBytes) {
  cfg::BufferPoolConfig poolConfig;
  // provide small shared buffer size
  // idea is to hit the limit and trigger XOFF (PFC)
  poolConfig.sharedBytes() = globalSharedBytes;
  poolConfig.headroomBytes() = globalHeadroomBytes;
  bufferPoolCfgMap.insert(std::make_pair("bufferNew", poolConfig));
}

void setupPortPgConfig(
    std::map<std::string, std::vector<cfg::PortPgConfig>>& portPgConfigMap,
    const std::vector<int>& losslessPgIds,
    int pgLimit,
    int pgHeadroom,
    std::optional<cfg::MMUScalingFactor> scalingFactor,
    int resumeOffset) {
  std::vector<cfg::PortPgConfig> portPgConfigs;
  // create 2 pgs
  for (auto pgId : losslessPgIds) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    pgConfig.bufferPoolName() = "bufferNew";
    // provide atleast 1 cell worth of minLimit
    pgConfig.minLimitBytes() = pgLimit;
    // set large enough headroom to avoid drop
    pgConfig.headroomLimitBytes() = pgHeadroom;
    // resume offset
    pgConfig.resumeOffsetBytes() = resumeOffset;
    // set scaling factor
    if (scalingFactor) {
      pgConfig.scalingFactor() = *scalingFactor;
    }
    portPgConfigs.emplace_back(pgConfig);
  }

  // create lossy pgs
  if (!FLAGS_allow_zero_headroom_for_lossless_pg) {
    // If the flag is set, we already have lossless PGs being created
    // with headroom as 0 and there is no way to differentiate lossy
    // and lossless PGs now that headroom is set to zero for lossless.
    // So, avoid creating lossy PGs as this will result in PFC being
    // enabled for 3 priorities, which is not supported for TAJO.
    for (auto pgId : kLossyPgIds) {
      cfg::PortPgConfig pgConfig;
      pgConfig.id() = pgId;
      pgConfig.bufferPoolName() = "bufferNew";
      // provide atleast 1 cell worth of minLimit
      pgConfig.minLimitBytes() = pgLimit;
      // headroom set 0 identifies lossy pgs
      pgConfig.headroomLimitBytes() = 0;
      // resume offset
      pgConfig.resumeOffsetBytes() = resumeOffset;
      // set scaling factor
      if (scalingFactor) {
        pgConfig.scalingFactor() = *scalingFactor;
      }
      portPgConfigs.emplace_back(pgConfig);
    }
  }

  portPgConfigMap["foo"] = std::move(portPgConfigs);
}

} // namespace

void setupPfcBuffers(
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports,
    const std::vector<int>& losslessPgIds,
    PfcBufferParams buffer) {
  setupPfc(cfg, ports);

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  setupPortPgConfig(
      portPgConfigMap,
      losslessPgIds,
      buffer.pgLimit,
      buffer.pgHeadroom,
      buffer.scalingFactor,
      buffer.resumeOffset);
  cfg.portPgConfigs() = std::move(portPgConfigMap);

  // create buffer pool
  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
  setupBufferPoolConfig(
      bufferPoolCfgMap, buffer.globalShared, buffer.globalHeadroom);
  cfg.bufferPoolConfigs() = std::move(bufferPoolCfgMap);
}

} // namespace facebook::fboss::utility
