// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

namespace facebook::fboss::utility {

cfg::Srv6Tunnel makeSrv6TunnelConfig(
    const std::string& name,
    InterfaceID interfaceId) {
  cfg::Srv6Tunnel tunnel;
  tunnel.srv6TunnelId() = name;
  tunnel.underlayIntfID() = interfaceId;
  tunnel.tunnelType() = TunnelType::SRV6_ENCAP;
  tunnel.srcIp() = "2001:db8::1";
  tunnel.ttlMode() = cfg::TunnelMode::PIPE;
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

} // namespace facebook::fboss::utility
