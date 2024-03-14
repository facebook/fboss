// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/TrapPacketUtils.h"

namespace facebook::fboss::utility {

void addTrapPacketAcl(cfg::SwitchConfig* config, PortID port) {
  cfg::AclEntry entry{};
  entry.name() = folly::to<std::string>("trap-packet-", port);
  entry.srcPort() = port;
  entry.actionType() = cfg::AclActionType::PERMIT;
  config->acls()->push_back(entry);

  cfg::MatchAction action;
  action.sendToQueue() = cfg::QueueMatchAction();
  action.sendToQueue()->queueId() = 0;
  action.toCpuAction() = cfg::ToCpuAction::TRAP;

  cfg::MatchToAction match2Action;
  match2Action.matcher() = entry.get_name();
  match2Action.action() = action;

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  if (config->dataPlaneTrafficPolicy()) {
    dataPlaneTrafficPolicy = *config->dataPlaneTrafficPolicy();
  }
  dataPlaneTrafficPolicy.matchToAction()->push_back(match2Action);
  config->dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
}

void addTrapPacketAcl(
    cfg::SwitchConfig* config,
    const std::set<PortID>& ports) {
  for (auto port : ports) {
    addTrapPacketAcl(config, port);
  }
}

void addTrapPacketAcl(
    cfg::SwitchConfig* config,
    const folly::CIDRNetwork& prefix) {
  cfg::AclEntry entry{};
  entry.name() = folly::to<std::string>("trap-", prefix.first.str());
  entry.dstIp() = prefix.first.str();
  entry.actionType() = cfg::AclActionType::PERMIT;
  config->acls()->push_back(entry);

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

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  if (config->dataPlaneTrafficPolicy()) {
    dataPlaneTrafficPolicy = *config->dataPlaneTrafficPolicy();
  }
  dataPlaneTrafficPolicy.matchToAction()->push_back(match2Action);
  config->dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
}

void addTrapPacketAcl(
    cfg::SwitchConfig* config,
    const std::set<folly::CIDRNetwork>& prefixs) {
  for (auto prefix : prefixs) {
    addTrapPacketAcl(config, prefix);
  }
}

void addTrapPacketAcl(cfg::SwitchConfig* config, uint16_t l4DstPort) {
  cfg::AclEntry entry{};
  entry.name() = folly::to<std::string>("trap-packet-", l4DstPort);
  entry.l4DstPort() = l4DstPort;
  entry.actionType() = cfg::AclActionType::PERMIT;
  config->acls()->push_back(entry);

  cfg::MatchAction action;
  action.sendToQueue() = cfg::QueueMatchAction();
  action.sendToQueue()->queueId() = 0;
  action.toCpuAction() = cfg::ToCpuAction::TRAP;

  cfg::MatchToAction match2Action;
  match2Action.matcher() = entry.get_name();
  match2Action.action() = action;

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  if (config->dataPlaneTrafficPolicy()) {
    dataPlaneTrafficPolicy = *config->dataPlaneTrafficPolicy();
  }
  dataPlaneTrafficPolicy.matchToAction()->push_back(match2Action);
  config->dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
}

} // namespace facebook::fboss::utility
