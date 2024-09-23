// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/MirrorTestUtils.h"

#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

namespace facebook::fboss::utility {

template <>
MirrorTestParams<folly::IPAddressV4> getMirrorTestParams<folly::IPAddressV4>() {
  return MirrorTestParams<folly::IPAddressV4>(
      folly::IPAddressV4("101.0.0.10"), // sender
      folly::IPAddressV4("201.0.0.10"), // receiver
      folly::IPAddressV4("101.0.0.11")); // erspan destination
}

template <>
MirrorTestParams<folly::IPAddressV6> getMirrorTestParams<folly::IPAddressV6>() {
  return MirrorTestParams<folly::IPAddressV6>(
      folly::IPAddressV6("101::10"), // sender
      folly::IPAddressV6("201::10"), // receiver
      folly::IPAddressV6("101::11")); // erspan destination
}

folly::IPAddress getSflowMirrorDestination(bool isV4) {
  return isV4 ? folly::IPAddress("101.101.101.101")
              : folly::IPAddress("2401:101:101::101");
}

/*
 * This configures a local/erspan mirror session.
 * Adds a tunnel config if the mirrorname is erspan.
 */
template <typename AddrT>
void addMirrorConfig(
    cfg::SwitchConfig* cfg,
    const AgentEnsemble& ensemble,
    const std::string& mirrorName,
    bool truncate,
    uint8_t dscp) {
  auto mirrorToPort = ensemble.masterLogicalPortIds(
      {cfg::PortType::INTERFACE_PORT})[kMirrorToPortIndex];
  auto params = getMirrorTestParams<AddrT>();
  cfg::MirrorDestination destination;
  destination.egressPort() = cfg::MirrorEgressPort();
  destination.egressPort()->logicalID_ref() = mirrorToPort;
  if (mirrorName == kIngressErspan || mirrorName == kEgressErspan) {
    cfg::MirrorTunnel tunnel;
    cfg::GreTunnel greTunnel;
    greTunnel.ip() = params.mirrorDestinationIp.str();
    tunnel.greTunnel() = greTunnel;
    destination.tunnel() = tunnel;
  }
  cfg::Mirror mirrorConfig;
  mirrorConfig.name() = mirrorName;
  mirrorConfig.destination() = destination;
  mirrorConfig.truncate() = truncate;
  mirrorConfig.dscp() = dscp;
  cfg->mirrors()->push_back(mirrorConfig);
}

template void addMirrorConfig<folly::IPAddressV4>(
    cfg::SwitchConfig* cfg,
    const AgentEnsemble& ensemble,
    const std::string& mirrorName,
    bool truncate,
    uint8_t dscp);
template void addMirrorConfig<folly::IPAddressV6>(
    cfg::SwitchConfig* cfg,
    const AgentEnsemble& ensemble,
    const std::string& mirrorName,
    bool truncate,
    uint8_t dscp);

void configureSflowMirror(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    bool truncate,
    const std::string& destinationIp,
    uint32_t udpDstPort) {
  cfg::SflowTunnel sflowTunnel;
  sflowTunnel.ip() = destinationIp;
  sflowTunnel.udpSrcPort() = 6545;
  sflowTunnel.udpDstPort() = udpDstPort;

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

void configureTrapAcl(cfg::SwitchConfig& config, PortID portId) {
  utility::addTrapPacketAcl(&config, portId);
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
