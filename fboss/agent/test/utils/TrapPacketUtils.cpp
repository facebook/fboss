// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

namespace facebook::fboss::utility {

namespace {

std::string trapAclTableName(const HwAsic* asic) {
  if (asic->isSupported(HwAsic::Feature::SWITCH_ATTR_PRE_INGRESS_ACL)) {
    return kDefaultPreIngressAclTable();
  }
  return kDefaultAclTable();
}

cfg::AclStage trapAclStage(const HwAsic* asic) {
  if (asic->isSupported(HwAsic::Feature::SWITCH_ATTR_PRE_INGRESS_ACL)) {
    return cfg::AclStage::PRE_INGRESS;
  }
  return cfg::AclStage::INGRESS;
}

} // namespace

void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    PortID port) {
  auto makeTrapAction = [](cfg::AclStage aclStage) {
    cfg::MatchAction action;
    // Pre-ingress ACL tables (e.g. P200) only support PACKET_ACTION (+
    // COUNTER). Omit SET_TC, sendToQueue, and userDefinedTrap which ingress
    // CoPP uses. P200: src-port match + TRAP (FORCE_TRAP_WITH_EVENT in SDK).
    if (aclStage == cfg::AclStage::PRE_INGRESS) {
      action.toCpuAction() = cfg::ToCpuAction::TRAP;
      return action;
    }
    action.sendToQueue() = cfg::QueueMatchAction();
    action.sendToQueue()->queueId() = 0;
    action.toCpuAction() = cfg::ToCpuAction::COPY;
    cfg::SetTcAction setTcAction = cfg::SetTcAction();
    setTcAction.tcValue() = 0;
    action.setTc() = setTcAction;
    cfg::UserDefinedTrapAction userDefinedTrap = cfg::UserDefinedTrapAction();
    userDefinedTrap.queueId() = 0;
    action.userDefinedTrap() = userDefinedTrap;
    return action;
  };

  cfg::TrafficPolicyConfig trafficPolicy;
  cfg::CPUTrafficPolicyConfig cpuTrafficPolicy;
  if (config->cpuTrafficPolicy()) {
    cpuTrafficPolicy = *config->cpuTrafficPolicy();
    if (cpuTrafficPolicy.trafficPolicy()) {
      trafficPolicy = *cpuTrafficPolicy.trafficPolicy();
    }
  }

  auto addEntry = [&](const std::string& name,
                      std::optional<cfg::EtherType> etherType,
                      std::optional<cfg::IpType> ipType,
                      const std::string& tableName,
                      cfg::AclStage aclStage) {
    cfg::AclEntry entry{};
    entry.name() = name;
    entry.srcPort() = port;
    if (etherType.has_value()) {
      entry.etherType() = *etherType;
    }
    if (ipType.has_value()) {
      entry.ipType() = *ipType;
    }
    entry.actionType() = cfg::AclActionType::PERMIT;
    utility::addAclEntry(config, entry, tableName, aclStage);
    cfg::MatchToAction match2Action;
    match2Action.matcher() = name;
    match2Action.action() = makeTrapAction(aclStage);
    trafficPolicy.matchToAction()->push_back(match2Action);
  };

  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_QUMRAN4D ||
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO4) {
    // Q4D/J4 (DNX) reject FIELD_SRC_PORT + FIELD_ACL_IP_TYPE=NON_IP. Per
    // Broadcom, install the source-port entry in both ACL tables: an
    // etherType=IPv4 copy in the default table and an etherType=IPv6 copy in
    // the IPv6 table, so it traps both v4 and v6 traffic on the port.
    addEntry(
        folly::to<std::string>("trap-packet-", port, "-v4"),
        cfg::EtherType::IPv4,
        std::nullopt,
        utility::kDefaultAclTable(),
        cfg::AclStage::INGRESS);
    addEntry(
        folly::to<std::string>("trap-packet-", port, "-v6"),
        cfg::EtherType::IPv6,
        std::nullopt,
        utility::kIpv6AclTable(),
        cfg::AclStage::INGRESS);
  } else {
    const auto tableName = trapAclTableName(asic);
    const auto aclStage = trapAclStage(asic);
    // If ASIC needs Ether Type to be passed in to disambiguate ACL entry, then
    // for SRC port matching, IP type should be set to NON_IP to match all
    // ingress packets in the ASIC SRC port. Chenab does not use ip type.
    std::optional<cfg::IpType> ipType;
    if (!asic->isSupported(HwAsic::Feature::SWITCH_ATTR_PRE_INGRESS_ACL) &&
        asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE) &&
        asic->getAsicVendor() != HwAsic::AsicVendor::ASIC_VENDOR_CHENAB) {
      ipType = cfg::IpType::NON_IP;
    }
    addEntry(
        folly::to<std::string>("trap-packet-", port),
        std::nullopt,
        ipType,
        tableName,
        aclStage);
  }

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
    const folly::CIDRNetwork& prefix,
    cfg::ToCpuAction toCpuAction) {
  cfg::AclEntry entry{};
  entry.name() = folly::to<std::string>("trap-", prefix.first.str());
  entry.dstIp() = folly::IPAddress::networkToString(prefix);
  if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
    entry.etherType() =
        prefix.first.isV6() ? cfg::EtherType::IPv6 : cfg::EtherType::IPv4;
  }
  entry.actionType() = cfg::AclActionType::PERMIT;
  utility::addAclEntry(config, entry, utility::kDefaultAclTable());

  cfg::MatchAction action;
  action.sendToQueue() = cfg::QueueMatchAction();
  action.sendToQueue()->queueId() = 0;
  action.toCpuAction() = toCpuAction;
  cfg::SetTcAction setTcAction = cfg::SetTcAction();
  setTcAction.tcValue() = 0;
  action.setTc() = setTcAction;

  cfg::UserDefinedTrapAction userDefinedTrap = cfg::UserDefinedTrapAction();
  userDefinedTrap.queueId() = 0;
  action.userDefinedTrap() = userDefinedTrap;

  cfg::MatchToAction match2Action;
  match2Action.matcher() = entry.name().value();
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
  match2Action.matcher() = entry.name().value();
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
