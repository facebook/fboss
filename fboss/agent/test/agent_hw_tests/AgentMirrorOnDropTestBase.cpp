/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTestBase.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTajoImpl.h"

#include <unordered_set>
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/HdrParseError.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"

namespace facebook::fboss {

cfg::SwitchConfig AgentMirrorOnDropTestBase::initialConfig(
    const AgentEnsemble& ensemble) const {
  auto config = utility::onePortPerInterfaceConfig(
      ensemble.getSw(),
      ensemble.masterLogicalPortIds(),
      true /*interfaceHasSubnet*/);
  // To allow infinite traffic loops.
  utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
  return config;
}

std::vector<ProductionFeature>
AgentMirrorOnDropTestBase::getProductionFeaturesVerified() const {
  return {ProductionFeature::MIRROR_ON_DROP};
}

std::string AgentMirrorOnDropTestBase::portDesc(const PortID& portId) const {
  const auto& cfg = getAgentEnsemble()->getCurrentConfig();
  for (const auto& port : *cfg.ports()) {
    if (PortID(*port.logicalID()) == portId) {
      return folly::sformat(
          "portId={} name={}", *port.logicalID(), *port.name());
    }
  }
  return "";
}

void AgentMirrorOnDropTestBase::setupEcmpTraffic(
    const PortID& portId,
    const folly::IPAddressV6& addr,
    const folly::MacAddress& nextHopMac,
    bool disableTtlDecrement) {
  utility::EcmpSetupTargetedPorts6 ecmpHelper{
      getProgrammedState(), getSw()->needL2EntryForNeighbor(), nextHopMac};
  const PortDescriptor port{portId};
  RoutePrefixV6 route{addr, 128};
  applyNewState([&](const std::shared_ptr<SwitchState>& state) {
    return ecmpHelper.resolveNextHops(state, {port});
  });
  auto routeUpdater = getSw()->getRouteUpdater();
  ecmpHelper.programRoutes(
      &routeUpdater,
      {port},
      {std::move(route)},
      {},
      std::nullopt,
      disableTtlDecrement ? std::make_optional(true) : std::nullopt);
  if (disableTtlDecrement) {
    for (auto& nhop : ecmpHelper.getNextHops()) {
      utility::disablePortTTLDecrementIfSupported(
          getAgentEnsemble(), ecmpHelper.getRouterId(), nhop);
    }
  }
}

void AgentMirrorOnDropTestBase::addDropPacketAcl(
    cfg::SwitchConfig* config,
    const PortID& portId) {
  auto* acl = utility::addAcl_DEPRECATED(
      config, fmt::format("drop-packet-{}", portId), cfg::AclActionType::DENY);
  acl->etherType() = cfg::EtherType::IPv6;
  acl->srcPort() = portId;
}

std::unique_ptr<TxPacket> AgentMirrorOnDropTestBase::makePacket(
    const folly::IPAddressV6& dstIp,
    int priority,
    size_t payloadSize) {
  // Create a structured payload pattern.
  std::vector<uint8_t> payload(payloadSize, 0);
  for (size_t i = 0; i < payload.size() / 2; ++i) {
    payload[i * 2] = i & 0xff;
    payload[i * 2 + 1] = (i >> 8) & 0xff;
  }

  return utility::makeUDPTxPacket(
      getSw(),
      getVlanIDForTx(),
      utility::kLocalCpuMac(),
      getMacForFirstInterfaceWithPortsForTesting(getProgrammedState()),
      kPacketSrcIp_,
      dstIp,
      kPacketSrcPort,
      kPacketDstPort,
      (priority * 8) << 2, // dscp is the 6 MSBs in TC
      255,
      std::move(payload));
}

std::unique_ptr<TxPacket> AgentMirrorOnDropTestBase::sendPackets(
    int count,
    std::optional<PortID> portId,
    const folly::IPAddressV6& dstIp,
    int priority,
    size_t payloadSize) {
  // Build a reference packet for the caller to compare against the captured
  // MoD payload. The reference is never sent — each loop iteration builds a
  // fresh packet because sendPacketAsync takes ownership.
  auto refPacket = makePacket(dstIp, priority, payloadSize);
  XLOG(DBG3) << "Sending " << count << " packets:\n"
             << PktUtil::hexDump(refPacket->buf());
  for (int i = 0; i < count; ++i) {
    if (portId.has_value()) {
      getAgentEnsemble()->sendPacketAsync(
          makePacket(dstIp, priority, payloadSize),
          PortDescriptor(*portId),
          std::nullopt);
    } else {
      getAgentEnsemble()->sendPacketAsync(
          makePacket(dstIp, priority, payloadSize), std::nullopt, std::nullopt);
    }
  }
  return refPacket;
}

SystemPortID AgentMirrorOnDropTestBase::systemPortId(
    const PortID& portId) const {
  return getSystemPortID(
      portId, getProgrammedState(), scopeResolver().scope(portId).switchId());
}

uint16_t AgentMirrorOnDropTestBase::expectedTajoModIngressPort(
    const PortID& portId,
    const HwSwitch* hw) const {
  try {
    return static_cast<uint16_t>(systemPortId(portId));
  } catch (const FbossError&) {
  }

  const auto state = getProgrammedState();
  const auto switchId = scopeResolver().scope(portId).switchId();
  const auto& infoMap = state->getSwitchSettings()
                            ->getSwitchSettings(HwSwitchMatcher(
                                std::unordered_set<SwitchID>{switchId}))
                            ->getSwitchIdToSwitchInfo();
  const auto infoIt = infoMap.find(static_cast<int64_t>(switchId));
  if (infoIt != infoMap.end() && infoIt->second.portIdRange().has_value()) {
    const auto portScope = state->getPorts()->getNode(portId)->getScope();
    const int32_t offset = portScope == cfg::Scope::GLOBAL
        ? infoIt->second.globalSystemPortOffset().value_or(0)
        : infoIt->second.localSystemPortOffset().value_or(0);
    const int64_t min = *infoIt->second.portIdRange()->minimum();
    return static_cast<uint16_t>(static_cast<int64_t>(portId) + offset - min);
  }

  TajoMirrorOnDropImpl tajoImpl;
  return tajoImpl.expectedIngressPort(hw, portId);
}

bool AgentMirrorOnDropTestBase::looksLikeMirrorOnDropOuterPacket(
    const folly::IOBuf* buf) {
  // Untagged Ethernet (14) + IPv6 (40) + UDP (8).
  static constexpr size_t kMinOuterModBytes = 14 + 40 + 8;
  if (!buf || buf->computeChainDataLength() < kMinOuterModBytes) {
    return false;
  }
  try {
    folly::io::Cursor cursor(buf);
    const auto eth = EthHdr(cursor);
    if (eth.getEtherType() !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    const auto ipv6 = IPv6Hdr(cursor);
    if (ipv6.dstAddr != kCollectorIp_) {
      return false;
    }
    if (ipv6.nextHeader != static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
      return false;
    }
    UDPHeader udp;
    udp.parse(&cursor);
    return udp.dstPort == static_cast<uint16_t>(kMirrorDstPort);
  } catch (const HdrParseError&) {
    return false;
  }
}

} // namespace facebook::fboss
