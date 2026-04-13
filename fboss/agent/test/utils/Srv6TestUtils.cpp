// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/Srv6TestUtils.h"
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

} // namespace facebook::fboss::utility
