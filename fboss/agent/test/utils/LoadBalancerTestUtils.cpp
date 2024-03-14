// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

#include "fboss/agent/LoadBalancerConfigApplier.h"
#include "fboss/agent/LoadBalancerUtils.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

#include <folly/gen/Base.h>

namespace {
constexpr uint8_t kDefaultQueue = 0;
}

namespace facebook::fboss::utility {

namespace {

cfg::Fields getHalfHashFields() {
  cfg::Fields hashFields;
  hashFields.ipv4Fields() = std::set<cfg::IPv4Field>(
      {cfg::IPv4Field::SOURCE_ADDRESS, cfg::IPv4Field::DESTINATION_ADDRESS});
  hashFields.ipv6Fields() = std::set<cfg::IPv6Field>(
      {cfg::IPv6Field::SOURCE_ADDRESS, cfg::IPv6Field::DESTINATION_ADDRESS});

  return hashFields;
}

cfg::Fields getFullHashFields() {
  auto hashFields = getHalfHashFields();
  hashFields.transportFields() = std::set<cfg::TransportField>(
      {cfg::TransportField::SOURCE_PORT,
       cfg::TransportField::DESTINATION_PORT});
  return hashFields;
}

cfg::Fields getFullHashUdf() {
  auto hashFields = getHalfHashFields();
  hashFields.transportFields() = std::set<cfg::TransportField>(
      {cfg::TransportField::SOURCE_PORT,
       cfg::TransportField::DESTINATION_PORT});
  hashFields.udfGroups() =
      std::vector<std::string>({kUdfHashDstQueuePairGroupName});
  return hashFields;
}

cfg::LoadBalancer getHalfHashConfig(
    const HwAsic& asic,
    cfg::LoadBalancerID id) {
  cfg::LoadBalancer loadBalancer;
  *loadBalancer.id() = id;
  if (asic.isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    *loadBalancer.fieldSelection() = getHalfHashFields();
  }
  *loadBalancer.algorithm() = cfg::HashingAlgorithm::CRC16_CCITT;
  return loadBalancer;
}
cfg::LoadBalancer getFullHashConfig(
    const HwAsic& asic,
    cfg::LoadBalancerID id) {
  cfg::LoadBalancer loadBalancer;
  *loadBalancer.id() = id;
  if (asic.isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    *loadBalancer.fieldSelection() = getFullHashFields();
  }
  *loadBalancer.algorithm() = cfg::HashingAlgorithm::CRC16_CCITT;
  return loadBalancer;
}
cfg::LoadBalancer getFullHashUdfConfig(
    const HwAsic& asic,
    cfg::LoadBalancerID id) {
  cfg::LoadBalancer loadBalancer;
  *loadBalancer.id() = id;
  if (asic.isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    *loadBalancer.fieldSelection() = getFullHashUdf();
  }
  *loadBalancer.algorithm() = cfg::HashingAlgorithm::CRC16_CCITT;
  return loadBalancer;
}
} // namespace
cfg::LoadBalancer getTrunkHalfHashConfig(const HwAsic& asic) {
  return getHalfHashConfig(asic, cfg::LoadBalancerID::AGGREGATE_PORT);
}
cfg::LoadBalancer getTrunkFullHashConfig(const HwAsic& asic) {
  return getFullHashConfig(asic, cfg::LoadBalancerID::AGGREGATE_PORT);
}
cfg::LoadBalancer getEcmpHalfHashConfig(const HwAsic& asic) {
  return getHalfHashConfig(asic, cfg::LoadBalancerID::ECMP);
}
cfg::LoadBalancer getEcmpFullHashConfig(const HwAsic& asic) {
  return getFullHashConfig(asic, cfg::LoadBalancerID::ECMP);
}
cfg::LoadBalancer getEcmpFullUdfHashConfig(const HwAsic& asic) {
  return getFullHashUdfConfig(asic, cfg::LoadBalancerID::ECMP);
}
std::vector<cfg::LoadBalancer> getEcmpFullTrunkHalfHashConfig(
    const HwAsic& asic) {
  return {getEcmpFullHashConfig(asic), getTrunkHalfHashConfig(asic)};
}
std::vector<cfg::LoadBalancer> getEcmpHalfTrunkFullHashConfig(
    const HwAsic& asic) {
  return {getEcmpHalfHashConfig(asic), getTrunkFullHashConfig(asic)};
}
std::vector<cfg::LoadBalancer> getEcmpFullTrunkFullHashConfig(
    const HwAsic& asic) {
  return {getEcmpFullHashConfig(asic), getTrunkFullHashConfig(asic)};
}

static cfg::UdfConfig addUdfConfig(
    std::map<std::string, cfg::UdfGroup>& udfMap,
    const std::string& udfGroup,
    const int offsetBytes,
    const int fieldSizeBytes,
    cfg::UdfGroupType udfType) {
  cfg::UdfConfig udfCfg;
  cfg::UdfGroup udfGroupEntry;
  cfg::UdfPacketMatcher matchCfg;
  std::map<std::string, cfg::UdfPacketMatcher> udfPacketMatcherMap;

  matchCfg.name() = kUdfL4UdpRocePktMatcherName;
  matchCfg.l4PktType() = cfg::UdfMatchL4Type::UDF_L4_PKT_TYPE_UDP;
  matchCfg.UdfL4DstPort() = kUdfL4DstPort;

  udfGroupEntry.name() = udfGroup;
  udfGroupEntry.header() = cfg::UdfBaseHeaderType::UDF_L4_HEADER;
  udfGroupEntry.startOffsetInBytes() = offsetBytes;
  udfGroupEntry.fieldSizeInBytes() = fieldSizeBytes;
  // has to be the same as in matchCfg
  udfGroupEntry.udfPacketMatcherIds() = {kUdfL4UdpRocePktMatcherName};
  udfGroupEntry.type() = udfType;

  udfMap.insert(std::make_pair(*udfGroupEntry.name(), udfGroupEntry));
  udfPacketMatcherMap.insert(std::make_pair(*matchCfg.name(), matchCfg));
  udfCfg.udfGroups() = udfMap;
  udfCfg.udfPacketMatcher() = udfPacketMatcherMap;
  return udfCfg;
}

cfg::UdfConfig addUdfAclConfig(void) {
  std::map<std::string, cfg::UdfGroup> udfMap;
  return addUdfConfig(
      udfMap,
      kUdfAclRoceOpcodeGroupName,
      kUdfAclRoceOpcodeStartOffsetInBytes,
      kUdfAclRoceOpcodeFieldSizeInBytes,
      cfg::UdfGroupType::ACL);
}

cfg::UdfConfig addUdfHashConfig(void) {
  std::map<std::string, cfg::UdfGroup> udfMap;
  return addUdfConfig(
      udfMap,
      kUdfHashDstQueuePairGroupName,
      kUdfHashDstQueuePairStartOffsetInBytes,
      kUdfHashDstQueuePairFieldSizeInBytes,
      cfg::UdfGroupType::HASH);
}

cfg::UdfConfig addUdfHashAclConfig(void) {
  std::map<std::string, cfg::UdfGroup> udfMap;
  addUdfConfig(
      udfMap,
      kUdfHashDstQueuePairGroupName,
      kUdfHashDstQueuePairStartOffsetInBytes,
      kUdfHashDstQueuePairFieldSizeInBytes,
      cfg::UdfGroupType::HASH);
  return addUdfConfig(
      udfMap,
      kUdfAclRoceOpcodeGroupName,
      kUdfAclRoceOpcodeStartOffsetInBytes,
      kUdfAclRoceOpcodeFieldSizeInBytes,
      cfg::UdfGroupType::ACL);
}

cfg::FlowletSwitchingConfig getDefaultFlowletSwitchingConfig(void) {
  cfg::FlowletSwitchingConfig flowletCfg;
  flowletCfg.inactivityIntervalUsecs() = 16;
  flowletCfg.flowletTableSize() = 2048;
  // set the egress load and queue exponent to zero for DLB engine
  // to do load balancing across all the links better with single stream
  flowletCfg.dynamicEgressLoadExponent() = 0;
  flowletCfg.dynamicQueueExponent() = 0;
  flowletCfg.dynamicQueueMinThresholdBytes() = 1000;
  flowletCfg.dynamicQueueMaxThresholdBytes() = 10000;
  flowletCfg.dynamicSampleRate() = 1000000;
  flowletCfg.dynamicEgressMinThresholdBytes() = 1000;
  flowletCfg.dynamicEgressMaxThresholdBytes() = 10000;
  flowletCfg.dynamicPhysicalQueueExponent() = 4;
  return flowletCfg;
}

void addFlowletAcl(cfg::SwitchConfig& cfg) {
  auto* acl = utility::addAcl(&cfg, "test-flowlet-acl");
  acl->proto() = 17;
  acl->l4DstPort() = 4791;
  acl->dstIp() = "2001::/16";
  cfg::MatchAction matchAction = cfg::MatchAction();
  matchAction.flowletAction() = cfg::FlowletAction::FORWARD;
  matchAction.counter() = "test-flowlet-acl-stats";
  std::vector<cfg::CounterType> counterTypes{
      cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
  auto counter = cfg::TrafficCounter();
  *counter.name() = "test-flowlet-acl-stats";
  *counter.types() = counterTypes;
  cfg.trafficCounters()->push_back(counter);
  utility::addMatcher(&cfg, "test-flowlet-acl", matchAction);
}

void addFlowletConfigs(
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports) {
  cfg::FlowletSwitchingConfig flowletCfg =
      utility::getDefaultFlowletSwitchingConfig();
  cfg.flowletSwitchingConfig() = flowletCfg;

  std::map<std::string, cfg::PortFlowletConfig> portFlowletCfgMap;
  cfg::PortFlowletConfig portFlowletConfig;
  portFlowletConfig.scalingFactor() = kScalingFactor;
  portFlowletConfig.loadWeight() = kLoadWeight;
  portFlowletConfig.queueWeight() = kQueueWeight;
  portFlowletCfgMap.insert(std::make_pair("default", portFlowletConfig));
  cfg.portFlowletConfigs() = portFlowletCfgMap;

  std::vector<PortID> portIds(ports.begin(), ports.begin() + ports.size());
  for (auto portId : portIds) {
    auto portCfg = utility::findCfgPort(cfg, portId);
    portCfg->flowletConfigName() = "default";
  }
  addFlowletAcl(cfg);
}

std::shared_ptr<SwitchState> setLoadBalancer(
    TestEnsembleIf* ensemble,
    const std::shared_ptr<SwitchState>& inputState,
    const cfg::LoadBalancer& loadBalancerCfg,
    const SwitchIdScopeResolver& resolver) {
  return addLoadBalancers(ensemble, inputState, {loadBalancerCfg}, resolver);
}

std::shared_ptr<SwitchState> addLoadBalancers(
    TestEnsembleIf* ensemble,
    const std::shared_ptr<SwitchState>& inputState,
    const std::vector<cfg::LoadBalancer>& loadBalancerCfgs,
    const SwitchIdScopeResolver& resolver) {
  bool sai = ensemble->isSai();
  auto mac = ensemble->getLocalMac(SwitchID(0));
  if (!ensemble->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    // configuring hash is not supported.
    XLOG(WARNING) << "load balancer configuration is not supported.";
    return inputState;
  }
  auto newState{inputState->clone()};
  auto lbMap = newState->getLoadBalancers()->modify(&newState);
  for (const auto& loadBalancerCfg : loadBalancerCfgs) {
    auto id = LoadBalancerConfigParser::parseLoadBalancerID(loadBalancerCfg);
    auto loadBalancer = LoadBalancerConfigParser(
                            utility::generateDeterministicSeed(id, mac, sai))
                            .parse(loadBalancerCfg);
    if (lbMap->getNodeIf(loadBalancer->getID())) {
      lbMap->updateNode(loadBalancer, resolver.scope(loadBalancer));
    } else {
      lbMap->addNode(loadBalancer, resolver.scope(loadBalancer));
    }
  }
  return newState;
}

template <typename PortIdT, typename PortStatsT>
bool isLoadBalancedImpl(
    const std::map<PortIdT, PortStatsT>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk) {
  auto ecmpPorts = folly::gen::from(portIdToStats) |
      folly::gen::map([](const auto& portIdAndStats) {
                     return portIdAndStats.first;
                   }) |
      folly::gen::as<std::vector<PortIdT>>();

  auto portBytes =
      folly::gen::from(portIdToStats) |
      folly::gen::map([&](const auto& portIdAndStats) {
        if constexpr (std::is_same_v<PortStatsT, HwPortStats>) {
          return *portIdAndStats.second.outBytes_();
        } else if constexpr (std::is_same_v<PortStatsT, HwSysPortStats>) {
          const auto& stats = portIdAndStats.second;
          return stats.queueOutBytes_()->at(kDefaultQueue) +
              stats.queueOutDiscardBytes_()->at(kDefaultQueue);
        }
        throw FbossError("Unsupported port stats type in isLoadBalancedImpl");
      }) |
      folly::gen::as<std::set<uint64_t>>();

  auto lowest = *portBytes.begin();
  auto highest = *portBytes.rbegin();
  XLOG(DBG0) << " Highest bytes: " << highest << " lowest bytes: " << lowest;
  if (!lowest) {
    return !highest && noTrafficOk;
  }
  if (!weights.empty()) {
    auto maxWeight = *(std::max_element(weights.begin(), weights.end()));
    for (auto i = 0; i < portIdToStats.size(); ++i) {
      uint64_t portOutBytes;
      if constexpr (std::is_same_v<PortStatsT, HwPortStats>) {
        portOutBytes = *portIdToStats.find(ecmpPorts[i])->second.outBytes_();
      } else if constexpr (std::is_same_v<PortStatsT, HwSysPortStats>) {
        const auto& stats = portIdToStats.find(ecmpPorts[i])->second;
        portOutBytes = stats.queueOutBytes_()->at(kDefaultQueue) +
            stats.queueOutDiscardBytes_()->at(kDefaultQueue);
      } else {
        throw FbossError("Unsupported port stats type in isLoadBalancedImpl");
      }
      auto weightPercent = (static_cast<float>(weights[i]) / maxWeight) * 100.0;
      auto portOutBytesPercent =
          (static_cast<float>(portOutBytes) / highest) * 100.0;
      auto percentDev = std::abs(weightPercent - portOutBytesPercent);
      // Don't tolerate a deviation of more than maxDeviationPct
      XLOG(DBG2) << "Percent Deviation: " << percentDev
                 << ", Maximum Deviation: " << maxDeviationPct;
      if (percentDev > maxDeviationPct) {
        return false;
      }
    }
  } else {
    auto percentDev = (static_cast<float>(highest - lowest) / lowest) * 100.0;
    // Don't tolerate a deviation of more than maxDeviationPct
    XLOG(DBG2) << "Percent Deviation: " << percentDev
               << ", Maximum Deviation: " << maxDeviationPct;
    if (percentDev > maxDeviationPct) {
      return false;
    }
  }
  return true;
}

template bool isLoadBalancedImpl<SystemPortID, HwSysPortStats>(
    const std::map<SystemPortID, HwSysPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

template bool isLoadBalancedImpl<PortID, HwPortStats>(
    const std::map<PortID, HwPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

template bool isLoadBalancedImpl<std::string, HwPortStats>(
    const std::map<std::string, HwPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

template bool isLoadBalancedImpl<std::string, HwSysPortStats>(
    const std::map<std::string, HwSysPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

} // namespace facebook::fboss::utility
