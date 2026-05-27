// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::utility {

cfg::Srv6Tunnel makeSrv6TunnelConfig(
    const std::string& name,
    InterfaceID interfaceId) {
  cfg::Srv6Tunnel tunnel;
  tunnel.srv6TunnelId() = name;
  tunnel.underlayIntfID() = interfaceId;
  tunnel.tunnelType() = TunnelType::SRV6_ENCAP;
  tunnel.srcIp() = "2001:db8::1";
  tunnel.ttlMode() = cfg::TunnelMode::UNIFORM;
  tunnel.dscpMode() = cfg::TunnelMode::UNIFORM;
  tunnel.ecnMode() = cfg::TunnelMode::UNIFORM;
  return tunnel;
}

cfg::SwitchConfig srv6EcmpInitialConfig(const AgentEnsemble& ensemble) {
  auto cfg = utility::onePortPerInterfaceConfig(
      ensemble.getSw(),
      ensemble.masterLogicalPortIds(),
      true /*interfaceHasSubnet*/);
  std::vector<cfg::Srv6Tunnel> tunnelList;
  auto numTunnels =
      std::min(static_cast<int>(cfg.interfaces()->size()), kSrv6MaxEcmpWidth);
  tunnelList.reserve(numTunnels);
  for (int i = 0; i < numTunnels; ++i) {
    tunnelList.push_back(makeSrv6TunnelConfig(
        folly::sformat("srv6Tunnel{}", i),
        InterfaceID(cfg.interfaces()[i].intfID().value())));
  }
  cfg.srv6Tunnels() = tunnelList;
  cfg.loadBalancers() = {
      getEcmpFullWithFlowLabelHashConfig(ensemble.getL3Asics())};
  return cfg;
}

void verifySrv6EcnMarking(
    AgentEnsemble* ensemble,
    PortID egressPort,
    const std::function<void()>& sendFloodPackets,
    const std::function<
        bool(const folly::IPAddressV6& dstAddr, uint8_t ecnBits)>&
        isEcnMarkedPacket) {
  auto* sw = ensemble->getSw();
  // Record baseline ECN counter
  auto beforeStats = ensemble->getLatestPortStats(egressPort);
  auto beforeEcnCounter = *beforeStats.outEcnCounter_();

  // Disable TX on egress port to build queue congestion
  setCreditWatchdogAndPortTx(ensemble, egressPort, false);

  // Flood ECN-capable packets (caller provides the packet construction)
  sendFloodPackets();

  // Create snooper to capture trapped packets
  SwSwitchPacketSnooper snooper(sw, "srv6EcnSnooper");
  snooper.ignoreUnclaimedRxPkts();

  // Re-enable TX — queued packets drain through pipeline
  setCreditWatchdogAndPortTx(ensemble, egressPort, true);

  // Verify ECN counter incremented
  WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(1000), {
    auto afterStats = ensemble->getLatestPortStats(egressPort);
    auto afterEcnCounter = *afterStats.outEcnCounter_();
    EXPECT_EVENTUALLY_GT(afterEcnCounter, beforeEcnCounter);
  });

  // Verify trapped packet has ECN CE marking
  bool foundEcnMarked = false;
  WITH_RETRIES({
    auto frameRx = snooper.waitForPacket(1);
    if (frameRx.has_value()) {
      folly::io::Cursor cursor((*frameRx).get());
      EthFrame frame(cursor);
      auto v6Payload = frame.v6PayLoad();
      if (v6Payload.has_value()) {
        auto v6Hdr = v6Payload->header();
        if (isEcnMarkedPacket(v6Hdr.dstAddr, v6Hdr.trafficClass & 0x3)) {
          foundEcnMarked = true;
        }
      }
    }
    EXPECT_EVENTUALLY_TRUE(foundEcnMarked);
  });
}

std::string portNameForConfig(const cfg::SwitchConfig& cfg, PortID portId) {
  for (const auto& port : *cfg.ports()) {
    if (*port.logicalID() == static_cast<int32_t>(portId) &&
        port.name().has_value() && !port.name()->empty()) {
      return *port.name();
    }
  }
  throw FbossError("No name found for port ", portId);
}

cfg::MySidConfig makeAdjacencyMySidConfig(
    const std::string& portName,
    const std::string& locatorPrefix,
    int functionId) {
  cfg::MySidConfig mySidConfig;
  mySidConfig.locatorPrefix() = locatorPrefix;
  cfg::MySidEntryConfig entry;
  cfg::AdjacencyMySidConfig adjConfig;
  adjConfig.portName() = portName;
  adjConfig.isV6() = true;
  entry.adjacency() = adjConfig;
  mySidConfig.entries()[functionId] = entry;
  return mySidConfig;
}

void waitForMySidResolveOrUnresolve(
    const std::function<std::shared_ptr<SwitchState>()>& getState,
    const folly::IPAddressV6& sidPrefix,
    uint8_t prefixLen,
    bool resolved) {
  auto sidKey =
      folly::to<std::string>(sidPrefix.str(), "/", static_cast<int>(prefixLen));
  WITH_RETRIES({
    std::shared_ptr<MySid> mySid;
    for (const auto& [_, mySidMap] : std::as_const(*getState()->getMySids())) {
      if (auto node = mySidMap->getNodeIf(sidKey)) {
        mySid = node;
        break;
      }
    }
    ASSERT_EVENTUALLY_NE(mySid, nullptr) << "mysid not in switch state";
    EXPECT_EVENTUALLY_EQ(mySid->getResolvedNextHopsId().has_value(), resolved)
        << "mysid " << sidKey
        << (resolved ? " not resolved" : " still resolved")
        << " (adjacencyInterfaceId="
        << (mySid->getAdjacencyInterfaceId().has_value()
                ? std::to_string(*mySid->getAdjacencyInterfaceId())
                : "<unset>")
        << ")";
  });
}

void addDecapMySidEntry(
    SwSwitch* sw,
    const folly::IPAddressV6& addr,
    uint8_t prefixLen) {
  MySidEntry entry;
  entry.type() = MySidType::DECAPSULATE_AND_LOOKUP;
  facebook::network::thrift::IPPrefix prefix;
  prefix.prefixAddress() = facebook::network::toBinaryAddress(addr);
  prefix.prefixLength() = prefixLen;
  entry.mySid() = prefix;

  auto rib = sw->getRib();
  auto ribMySidToSwitchStateFunc =
      createRibMySidToSwitchStateFunction(std::nullopt);
  rib->update(
      sw->getScopeResolver(),
      {entry},
      {},
      "addDecapMySidEntry",
      ribMySidToSwitchStateFunc,
      sw);
}

void addBindingSidEntry(
    SwSwitch* sw,
    const folly::IPAddressV6& mySidAddr,
    uint8_t prefixLen,
    const std::vector<NextHopThrift>& nhops) {
  ThriftHandler handler(sw);
  auto entries = std::make_unique<std::vector<MySidEntry>>();
  MySidEntry bindingEntry;
  bindingEntry.type() = MySidType::BINDING_MICRO_SID;
  facebook::network::thrift::IPPrefix mySidPrefix;
  mySidPrefix.prefixAddress() = facebook::network::toBinaryAddress(mySidAddr);
  mySidPrefix.prefixLength() = prefixLen;
  bindingEntry.mySid() = mySidPrefix;
  bindingEntry.nextHops() = nhops;
  entries->push_back(bindingEntry);
  handler.addMySidEntries(std::move(entries));
}

NextHopThrift makeSrv6NextHopThrift(
    const folly::IPAddressV6& nhopAddr,
    const folly::IPAddressV6& sid,
    const std::string& tunnelId) {
  NextHopThrift nhop;
  nhop.address() = facebook::network::toBinaryAddress(nhopAddr);
  nhop.srv6SegmentList() = {facebook::network::toBinaryAddress(sid)};
  nhop.tunnelType() = TunnelType::SRV6_ENCAP;
  nhop.tunnelId() = tunnelId;
  return nhop;
}

} // namespace facebook::fboss::utility
