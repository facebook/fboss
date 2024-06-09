// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/MirrorTestUtils.h"

#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

namespace facebook::fboss::utility {

folly::IPAddress getSflowMirrorDestination(bool isV4) {
  return isV4 ? folly::IPAddress("101.101.101.101")
              : folly::IPAddress("2401:101:101::101");
}

void configureSflowMirror(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    bool truncate,
    bool isV4) {
  cfg::SflowTunnel sflowTunnel;
  sflowTunnel.ip() = getSflowMirrorDestination(isV4).str();
  sflowTunnel.udpSrcPort() = 6545;
  sflowTunnel.udpDstPort() = 6343;

  cfg::MirrorTunnel tunnel;
  tunnel.sflowTunnel() = sflowTunnel;

  cfg::MirrorDestination destination;
  destination.tunnel() = tunnel;

  cfg::Mirror mirror;
  mirror.name() = mirrorName;
  mirror.destination() = destination;
  mirror.truncate() = truncate;

  config.mirrors()->push_back(mirror);
}

void configureTrapAcl(cfg::SwitchConfig& config, bool isV4) {
  folly::CIDRNetwork cidr{
      getSflowMirrorDestination(isV4).str(), (isV4 ? 32 : 128)};
  utility::addTrapPacketAcl(&config, cidr);
}

void configureSflowSampling(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    const std::vector<PortID>& ports,
    int sampleRate) {
  for (const auto& port : ports) {
    auto portCfg = utility::findCfgPort(config, port);
    portCfg->sFlowIngressRate() = sampleRate;
    portCfg->sampleDest() = cfg::SampleDestination::MIRROR;
    portCfg->ingressMirror() = mirrorName;
  }
}

} // namespace facebook::fboss::utility
