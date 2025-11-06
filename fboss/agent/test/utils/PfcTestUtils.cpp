// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/PfcTestUtils.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "folly/MacAddress.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

namespace {

void setupQosMapForPfc(
    cfg::QosMap& qosMap,
    bool isCpuQosMap,
    const std::map<int, int>& tc2PgOverride = {}) {
  // update pfc maps
  std::map<int16_t, int16_t> tc2PgId;
  std::map<int16_t, int16_t> tc2QueueId;
  std::map<int16_t, int16_t> pfcPri2PgId;
  std::map<int16_t, int16_t> pfcPri2QueueId;
  // program defaults
  for (auto i = 0; i < 8; i++) {
    tc2PgId.emplace(i, i);
    if (isCpuQosMap) {
      // Jericho3 cpu/recycle port only has 2 egress queues. Tomahawk has more
      // queues, but we stick to the lowest common denominator here.
      // See https://fburl.com/gdoc/nyyg1cve and https://fburl.com/code/mhdeuiky
      tc2QueueId.emplace(i, i < 7 ? 0 : 1);
    } else {
      tc2QueueId.emplace(i, i);
    }
    pfcPri2PgId.emplace(i, i);
    pfcPri2QueueId.emplace(i, i);
  }

  for (auto& tc2Pg : tc2PgOverride) {
    tc2PgId[tc2Pg.first] = tc2Pg.second;
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

void setupPfc(
    const TestEnsembleIf* ensemble,
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports,
    const std::map<int, int>& tcToPgOverride) {
  cfg::PortPfc pfc;
  pfc.tx() = true;
  pfc.rx() = true;
  pfc.portPgConfigName() = "foo";

  for (const auto& portID : ports) {
    auto portCfg = std::find_if(
        cfg.ports()->begin(), cfg.ports()->end(), [&portID](auto& port) {
          return PortID(*port.logicalID()) == portID;
        });
    portCfg->pfc() = pfc;
  }

  // setup qosPolicy
  auto setupQosPolicy = [&](bool isCpuQosMap, const std::string& name) {
    cfg::QosMap qosMap;
    setupQosMapForPfc(qosMap, isCpuQosMap, tcToPgOverride);
    auto qosPolicy = cfg::QosPolicy();
    *qosPolicy.name() = name;
    qosPolicy.qosMap() = std::move(qosMap);

    // add or replace existing policy (don't add the same policy twice!)
    bool found = false;
    for (auto& existing : *cfg.qosPolicies()) {
      if (existing.name() == name) {
        existing = qosPolicy;
        found = true;
        break;
      }
    }
    if (!found) {
      cfg.qosPolicies()->push_back(qosPolicy);
    }

    cfg::TrafficPolicyConfig trafficPolicy;
    trafficPolicy.defaultQosPolicy() = name;
    return trafficPolicy;
  };
  auto dataTrafficPolicy = setupQosPolicy(false /*isCpuQosMap*/, "qp");
  if (ensemble->getHwAsicTable()
          ->getHwAsics()
          .cbegin()
          ->second->getSwitchType() == cfg::SwitchType::VOQ) {
    // Start with the current CPU traffic policy, overwrite whats
    // needed here, leave the rest as is!
    cfg::CPUTrafficPolicyConfig cpuPolicy = cfg.cpuTrafficPolicy()
        ? *cfg.cpuTrafficPolicy()
        : cfg::CPUTrafficPolicyConfig();
    const std::string kCpuQueueingPolicy{"cpuQp"};
    cpuPolicy.trafficPolicy() =
        setupQosPolicy(true /*isCpuQosMap*/, kCpuQueueingPolicy);
    cfg.cpuTrafficPolicy() = std::move(cpuPolicy);
    std::map<int, std::string> portIdToQosPolicy{};
    for (const auto& portId : ensemble->masterLogicalPortIds(
             {cfg::PortType::CPU_PORT, cfg::PortType::RECYCLE_PORT})) {
      portIdToQosPolicy[static_cast<int>(portId)] = kCpuQueueingPolicy;
    }
    if (portIdToQosPolicy.size()) {
      dataTrafficPolicy.portIdToQosPolicy() = std::move(portIdToQosPolicy);
    }
  }
  cfg.dataPlaneTrafficPolicy() = dataTrafficPolicy;
}

void setupBufferPoolConfig(
    const HwAsic* asic,
    std::map<std::string, cfg::BufferPoolConfig>& bufferPoolCfgMap,
    int globalSharedBytes,
    int globalHeadroomBytes) {
  cfg::BufferPoolConfig poolConfig;
  // provide small shared buffer size
  // idea is to hit the limit and trigger XOFF (PFC)
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
    // Round up the configured buffer size to the nearest multiple of unit size
    auto unit = asic->getPacketBufferUnitSize();
    auto roundUp = [unit](int size) {
      return std::ceil(static_cast<double>(size) / unit) * unit;
    };
    poolConfig.sharedBytes() = roundUp(globalSharedBytes);
    poolConfig.headroomBytes() = roundUp(globalHeadroomBytes);
  } else {
    poolConfig.sharedBytes() = globalSharedBytes;
    poolConfig.headroomBytes() = globalHeadroomBytes;
  }
  bufferPoolCfgMap.insert(std::make_pair("bufferNew", poolConfig));
}

void setupPortPgConfig(
    const TestEnsembleIf* ensemble,
    std::map<std::string, std::vector<cfg::PortPgConfig>>& portPgConfigMap,
    const std::vector<int>& losslessPgIds,
    const std::vector<int>& lossyPgIds,
    const PfcBufferParams& buffer) {
  std::vector<cfg::PortPgConfig> portPgConfigs;

  for (auto pgId : losslessPgIds) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    pgConfig.bufferPoolName() = "bufferNew";
    pgConfig.minLimitBytes() = buffer.minLimit;
    // set large enough headroom to avoid drop
    pgConfig.headroomLimitBytes() = buffer.pgHeadroom;
    // resume threshold/offset
    if (buffer.resumeThreshold.has_value()) {
      pgConfig.resumeBytes() = *buffer.resumeThreshold;
    }
    if (buffer.resumeOffset.has_value()) {
      pgConfig.resumeOffsetBytes() = *buffer.resumeOffset;
    }
    if (ensemble->getHwAsicTable()
            ->getHwAsics()
            .cbegin()
            ->second->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      // Need to translate global config to shared thresholds
      pgConfig.maxSharedXoffThresholdBytes() = buffer.globalShared;
      pgConfig.minSharedXoffThresholdBytes() = buffer.globalShared;
      // Set some default values for SRAM thresholds
      pgConfig.maxSramXoffThresholdBytes() = 2048 * 16 * 256;
      pgConfig.minSramXoffThresholdBytes() = 256 * 16 * 256;
      pgConfig.sramResumeOffsetBytes() = 128 * 16 * 256;
      pgConfig.sramScalingFactor() = cfg::MMUScalingFactor::ONE_HALF;
    }
    // set static/dynamic thresholds
    if (buffer.pgShared.has_value()) {
      pgConfig.staticLimitBytes() = *buffer.pgShared;
    } else {
      pgConfig.scalingFactor() = buffer.scalingFactor;
    }

    portPgConfigs.emplace_back(pgConfig);
  }

  for (auto pgId : lossyPgIds) {
    cfg::PortPgConfig pgConfig;
    pgConfig.id() = pgId;
    pgConfig.bufferPoolName() = "bufferNew";
    pgConfig.minLimitBytes() = buffer.minLimit;
    // headroom set 0 identifies lossy pgs
    pgConfig.headroomLimitBytes() = 0;
    // set static/dynamic thresholds
    if (buffer.pgShared.has_value()) {
      pgConfig.staticLimitBytes() = *buffer.pgShared;
    } else {
      pgConfig.scalingFactor() = buffer.scalingFactor;
    }

    portPgConfigs.emplace_back(pgConfig);
  }

  portPgConfigMap["foo"] = std::move(portPgConfigs);
}

} // namespace

PfcBufferParams PfcBufferParams::getPfcBufferParams(
    cfg::AsicType asicType,
    int globalShared,
    int globalHeadroom) {
  PfcBufferParams buffer;
  buffer.globalShared = globalShared;
  buffer.globalHeadroom = globalHeadroom;

  if (asicType == cfg::AsicType::ASIC_TYPE_CHENAB) {
    // For CHENAB:
    // - XON represents the "min guarantee", must be at least 2xMTU (20480).
    // - RESERVED represents the total amount of buffer exclusively reserved
    //   for the PG.
    //   - In shared headroom pool mode (SAI_BUFFER_POOL_XOFF_SIZE > 0), this
    //     must be the same as XON.
    //   - In non-shared headroom pool mode (SAI_BUFFER_POOL_XOFF_SIZE == 0),
    //     this is the sum of XON and headroom size. Note that XOFF can be
    //     less than the headroom size, in which case there will be hystersis.
    if (globalHeadroom > 0) {
      buffer.resumeThreshold = 20480;
      buffer.pgHeadroom = 4400;
      buffer.minLimit = *buffer.resumeThreshold;
    } else {
      buffer.resumeThreshold = 20480;
      // TODO(maxgg): Understand why PFC won't trigger if this is < ~8000.
      buffer.pgHeadroom = 8000;
      buffer.minLimit = *buffer.resumeThreshold + buffer.pgHeadroom;
    }
  } else {
    buffer.minLimit = 2200;
    buffer.pgHeadroom = 2200; // keep this lower than globalShared (why?)
    buffer.resumeOffset = 1800; // less than pgHeadroom
  }

  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      buffer.globalShared = kSmallGlobalSharedBytes;
      break;
    default:
      break;
  }

  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      buffer.scalingFactor = cfg::MMUScalingFactor::ONE_HALF;
      break;
    default:
      buffer.scalingFactor = cfg::MMUScalingFactor::ONE_128TH;
      break;
  }

  return buffer;
}

void setupPfcBuffers(
    const TestEnsembleIf* ensemble,
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports,
    const std::vector<int>& losslessPgIds,
    const std::vector<int>& lossyPgIds,
    const std::map<int, int>& tcToPgOverride) {
  auto asicType = checkSameAndGetAsicType(cfg);
  setupPfcBuffers(
      ensemble,
      cfg,
      ports,
      losslessPgIds,
      lossyPgIds,
      tcToPgOverride,
      PfcBufferParams::getPfcBufferParams(asicType));
}

void setupPfcBuffers(
    const TestEnsembleIf* ensemble,
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports,
    const std::vector<int>& losslessPgIds,
    const std::vector<int>& lossyPgIds,
    const std::map<int, int>& tcToPgOverride,
    PfcBufferParams buffer) {
  setupPfc(ensemble, cfg, ports, tcToPgOverride);

  std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
  setupPortPgConfig(
      ensemble, portPgConfigMap, losslessPgIds, lossyPgIds, buffer);
  cfg.portPgConfigs() = std::move(portPgConfigMap);

  // create buffer pool
  std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap =
      cfg.bufferPoolConfigs().ensure();
  auto asic = checkSameAndGetAsic(ensemble->getL3Asics());
  setupBufferPoolConfig(
      asic, bufferPoolCfgMap, buffer.globalShared, buffer.globalHeadroom);
  cfg.bufferPoolConfigs() = std::move(bufferPoolCfgMap);
  if (ensemble->getHwAsicTable()
          ->getHwAsics()
          .cbegin()
          ->second->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // For J3, set the SRAM global PFC thresholds as well
    cfg.switchSettings()->sramGlobalFreePercentXoffThreshold() = 10;
    cfg.switchSettings()->sramGlobalFreePercentXonThreshold() = 20;
    cfg.switchSettings()->linkFlowControlCreditThreshold() = 99;
  }
}

void addPuntPfcPacketAcl(cfg::SwitchConfig& cfg, uint16_t queueId) {
  cfg::AclEntry entry;
  entry.name() = "pfcMacEntry";
  entry.actionType() = cfg::AclActionType::PERMIT;
  entry.dstMac() = "01:80:C2:00:00:01";
  utility::addAclEntry(&cfg, entry, utility::kDefaultAclTable());

  cfg::MatchToAction matchToAction;
  matchToAction.matcher() = "pfcMacEntry";
  cfg::MatchAction& action = matchToAction.action().ensure();
  action.toCpuAction() = cfg::ToCpuAction::TRAP;
  action.sendToQueue().ensure().queueId() = queueId;
  action.setTc().ensure().tcValue() = queueId;
  cfg.cpuTrafficPolicy()
      .ensure()
      .trafficPolicy()
      .ensure()
      .matchToAction()
      .ensure()
      .push_back(matchToAction);
}

std::string pfcStatsString(const HwPortStats& stats) {
  std::stringstream ss;
  ss << "outBytes=" << *stats.outBytes_() << " inBytes=" << *stats.inBytes_()
     << " outUnicastPkts=" << *stats.outUnicastPkts_()
     << " inUnicastPkts=" << *stats.inUnicastPkts_()
     << " inDiscards=" << *stats.inDiscards_()
     << " inDiscardsRaw=" << *stats.inDiscardsRaw_()
     << " inCongestionDiscards=" << *stats.inCongestionDiscards_()
     << " inErrors=" << *stats.inErrors_();
  for (auto [qos, value] : *stats.inPfc_()) {
    ss << " inPfc." << qos << "=" << value;
  }
  for (auto [qos, value] : *stats.outPfc_()) {
    ss << " outPfc." << qos << "=" << value;
  }
  return ss.str();
}

std::unique_ptr<TxPacket> makePfcFramePacket(
    const AgentEnsemble& ensemble,
    uint8_t classVector) {
  // Construct PFC payload with fixed quanta 0x00F0.
  // See https://github.com/archjeb/pfctest for frame structure.
  std::vector<uint8_t> payload{
      0x01, 0x01, 0x00, classVector, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0,
      0x00, 0xF0, 0x00, 0xF0,        0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0,
  };
  std::vector<uint8_t> padding(26, 0);
  payload.insert(payload.end(), padding.begin(), padding.end());

  // Construct PFC frame packet
  std::optional<VlanID> vlanId = ensemble.getVlanIDForTx();
  folly::MacAddress intfMac =
      utility::getMacForFirstInterfaceWithPorts(ensemble.getProgrammedState());
  MacAddressGenerator::ResourceT srcMac =
      utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
  return utility::makeEthTxPacket(
      ensemble.getSw(),
      vlanId,
      srcMac,
      folly::MacAddress("01:80:C2:00:00:01"), // MAC control address
      ETHERTYPE::ETHERTYPE_EPON, // Ethertype for PFC frames
      std::move(payload));
}

} // namespace facebook::fboss::utility
