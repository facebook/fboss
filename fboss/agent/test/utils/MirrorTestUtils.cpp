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

folly::IPAddress getSflowMirrorSource(bool isV4) {
  /*
   * This is the source IP for sflow mirror packets.
   * We will be supporting only v6 on future platforms.
   */
  return isV4 ? folly::IPAddress("100.100.100.100")
              : folly::IPAddress("2401::100");
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
    uint8_t dscp,
    uint16_t mirrorToPortIndex) {
  PortID mirrorToPort;
  if (FLAGS_hyper_port) {
    CHECK_LE(mirrorToPortIndex, ensemble.masterLogicalHyperPortIds().size());
    mirrorToPort = ensemble.masterLogicalPortIds(
        {cfg::PortType::HYPER_PORT})[mirrorToPortIndex];
  } else {
    CHECK_LE(
        mirrorToPortIndex, ensemble.masterLogicalInterfacePortIds().size());
    mirrorToPort = ensemble.masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[mirrorToPortIndex];
  }
  auto params = getMirrorTestParams<AddrT>();
  cfg::MirrorDestination destination;
  destination.egressPort() = cfg::MirrorEgressPort();
  destination.egressPort()->logicalID() = mirrorToPort;
  if (mirrorName.compare(0, kIngressErspan.length(), kIngressErspan) == 0 ||
      mirrorName.compare(0, kEgressErspan.length(), kEgressErspan) == 0) {
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
    uint8_t dscp,
    uint16_t mirrorToPortIndex);
template void addMirrorConfig<folly::IPAddressV6>(
    cfg::SwitchConfig* cfg,
    const AgentEnsemble& ensemble,
    const std::string& mirrorName,
    bool truncate,
    uint8_t dscp,
    uint16_t mirrorToPortIndex);

void configureSflowMirror(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    bool truncate,
    const std::string& destinationIp,
    uint32_t udpSrcPort,
    uint32_t udpDstPort,
    bool isV4,
    std::optional<int> sampleRate) {
  cfg::SflowTunnel sflowTunnel;
  sflowTunnel.ip() = destinationIp;
  sflowTunnel.udpSrcPort() = udpSrcPort;
  sflowTunnel.udpDstPort() = udpDstPort;

  cfg::MirrorTunnel tunnel;
  tunnel.srcIp() = getSflowMirrorSource(isV4).str();
  tunnel.sflowTunnel() = sflowTunnel;

  cfg::MirrorDestination destination;
  destination.tunnel() = tunnel;

  cfg::Mirror mirror;
  mirror.name() = mirrorName;
  mirror.destination() = destination;
  mirror.truncate() = truncate;
  if (sampleRate.has_value()) {
    mirror.samplingRate() = *sampleRate;
  }

  config.mirrors()->push_back(mirror);
}

void configureTrapAcl(
    const HwAsic* asic,
    cfg::SwitchConfig& config,
    bool isV4) {
  folly::CIDRNetwork cidr{
      getSflowMirrorDestination(isV4).str(), (isV4 ? 32 : 128)};
  utility::addTrapPacketAcl(asic, &config, cidr);
}

void configureTrapAcl(
    const HwAsic* asic,
    cfg::SwitchConfig& config,
    PortID portId) {
  utility::addTrapPacketAcl(asic, &config, portId);
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
