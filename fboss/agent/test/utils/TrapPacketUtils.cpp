// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

namespace facebook::fboss::utility {

void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    PortID port) {
  cfg::AclEntry entry{};
  entry.name() = folly::to<std::string>("trap-packet-", port);
  entry.srcPort() = port;
  // If ASIC needs Ether Type to be passed in to disambiguate ACL entry,
  // then for SRC port matching, IP type should be set to NON_IP to match
  // all ingress packets in the ASIC SRC port.
  if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
    entry.ipType() = cfg::IpType::NON_IP;
  }
  entry.actionType() = cfg::AclActionType::PERMIT;
  utility::addAclEntry(config, entry, utility::kDefaultAclTable());

  cfg::MatchAction action;
  action.sendToQueue() = cfg::QueueMatchAction();
  action.sendToQueue()->queueId() = 0;
  action.toCpuAction() = cfg::ToCpuAction::COPY;
  cfg::SetTcAction setTcAction = cfg::SetTcAction();
  setTcAction.tcValue() = 0;
  action.setTc() = setTcAction;

  cfg::UserDefinedTrapAction userDefinedTrap = cfg::UserDefinedTrapAction();
  userDefinedTrap.queueId() = 0;
  action.userDefinedTrap() = userDefinedTrap;

  cfg::MatchToAction match2Action;
  match2Action.matcher() = entry.get_name();
  match2Action.action() = action;

  cfg::TrafficPolicyConfig trafficPolicy;
  cfg::CPUTrafficPolicyConfig cpuTrafficPolicy;
  if (config->cpuTrafficPolicy()) {
    cpuTrafficPolicy = *config->cpuTrafficPolicy();
    if (cpuTrafficPolicy.trafficPolicy()) {
      trafficPolicy = *cpuTrafficPolicy.trafficPolicy();
    }
  }
  trafficPolicy.matchToAction()->push_back(match2Action);
  cpuTrafficPolicy.trafficPolicy() = trafficPolicy;
  config->cpuTrafficPolicy() = cpuTrafficPolicy;
}

void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    const std::set<PortID>& ports) {
  for (auto port : ports) {
    addTrapPacketAcl(asic, config, port);
  }
}

void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    const folly::CIDRNetwork& prefix) {
  cfg::AclEntry entry{};
  entry.name() = folly::to<std::string>("trap-", prefix.first.str());
  entry.dstIp() = prefix.first.str();
  if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
    entry.etherType() = cfg::EtherType::IPv6;
  }
  entry.actionType() = cfg::AclActionType::PERMIT;
  utility::addAclEntry(config, entry, utility::kDefaultAclTable());

  cfg::MatchAction action;
  action.sendToQueue() = cfg::QueueMatchAction();
  action.sendToQueue()->queueId() = 0;
  action.toCpuAction() = cfg::ToCpuAction::COPY;
  cfg::SetTcAction setTcAction = cfg::SetTcAction();
  setTcAction.tcValue() = 0;
  action.setTc() = setTcAction;

  cfg::UserDefinedTrapAction userDefinedTrap = cfg::UserDefinedTrapAction();
  userDefinedTrap.queueId() = 0;
  action.userDefinedTrap() = userDefinedTrap;

  cfg::MatchToAction match2Action;
  match2Action.matcher() = entry.get_name();
  match2Action.action() = action;

  cfg::TrafficPolicyConfig trafficPolicy;
  cfg::CPUTrafficPolicyConfig cpuTrafficPolicy;
  if (config->cpuTrafficPolicy()) {
    cpuTrafficPolicy = *config->cpuTrafficPolicy();
    if (cpuTrafficPolicy.trafficPolicy()) {
      trafficPolicy = *cpuTrafficPolicy.trafficPolicy();
    }
  }
  trafficPolicy.matchToAction()->push_back(match2Action);
  cpuTrafficPolicy.trafficPolicy() = trafficPolicy;
  config->cpuTrafficPolicy() = cpuTrafficPolicy;
}

void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    const std::set<folly::CIDRNetwork>& prefixs) {
  for (auto prefix : prefixs) {
    addTrapPacketAcl(asic, config, prefix);
  }
}

void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    uint16_t l4DstPort) {
  cfg::AclEntry entry{};
  entry.name() = folly::to<std::string>("trap-packet-", l4DstPort);
  entry.l4DstPort() = l4DstPort;
  if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
    entry.etherType() = cfg::EtherType::IPv6;
  }
  entry.actionType() = cfg::AclActionType::PERMIT;
  utility::addAclEntry(config, entry, utility::kDefaultAclTable());

  cfg::MatchAction action;
  action.sendToQueue() = cfg::QueueMatchAction();
  action.sendToQueue()->queueId() = 0;
  action.toCpuAction() = cfg::ToCpuAction::COPY;
  cfg::SetTcAction setTcAction = cfg::SetTcAction();
  setTcAction.tcValue() = 0;
  action.setTc() = setTcAction;

  cfg::UserDefinedTrapAction userDefinedTrap = cfg::UserDefinedTrapAction();
  userDefinedTrap.queueId() = 0;
  action.userDefinedTrap() = userDefinedTrap;

  cfg::MatchToAction match2Action;
  match2Action.matcher() = entry.get_name();
  match2Action.action() = action;

  cfg::TrafficPolicyConfig trafficPolicy;
  cfg::CPUTrafficPolicyConfig cpuTrafficPolicy;
  if (config->cpuTrafficPolicy()) {
    cpuTrafficPolicy = *config->cpuTrafficPolicy();
    if (cpuTrafficPolicy.trafficPolicy()) {
      trafficPolicy = *cpuTrafficPolicy.trafficPolicy();
    }
  }
  trafficPolicy.matchToAction()->push_back(match2Action);
  cpuTrafficPolicy.trafficPolicy() = trafficPolicy;
  config->cpuTrafficPolicy() = cpuTrafficPolicy;
}

void addTrapPacketAcl(cfg::SwitchConfig* config, folly::MacAddress mac) {
  cfg::AclEntry entry;
  entry.name() = folly::to<std::string>("trap-packet-", mac.toString());
  entry.dstMac() = mac.toString();
  entry.actionType() = cfg::AclActionType::PERMIT;
  utility::addAclEntry(config, entry, utility::kDefaultAclTable());

  cfg::MatchToAction matchToAction;
  matchToAction.matcher() = *entry.name();
  cfg::MatchAction& action = matchToAction.action().ensure();
  action.toCpuAction() = cfg::ToCpuAction::TRAP;
  action.sendToQueue().ensure().queueId() = 0;
  action.setTc().ensure().tcValue() = 0;
  config->cpuTrafficPolicy()
      .ensure()
      .trafficPolicy()
      .ensure()
      .matchToAction()
      .ensure()
      .push_back(matchToAction);
}

} // namespace facebook::fboss::utility
