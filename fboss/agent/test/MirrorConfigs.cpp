// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/MirrorConfigs.h"

namespace facebook::fboss::utility {
cfg::MirrorEgressPort getMirrorEgressPort(const std::string& portName) {
  cfg::MirrorEgressPort egressPort;
  egressPort.set_name(portName);
  return egressPort;
}

cfg::MirrorEgressPort getMirrorEgressPort(PortID portID) {
  cfg::MirrorEgressPort egressPort;
  egressPort.set_logicalID(portID);
  return egressPort;
}

cfg::Mirror getSPANMirror(
    const std::string& name,
    PortID portID,
    uint8_t dscp,
    bool truncate) {
  cfg::Mirror mirror;
  mirror.set_name(name);
  mirror.destination.set_egressPort(getMirrorEgressPort(portID));
  mirror.set_dscp(dscp);
  mirror.set_truncate(truncate);
  return mirror;
}

cfg::Mirror getSPANMirror(
    const std::string& name,
    const std::string& portName,
    uint8_t dscp,
    bool truncate) {
  cfg::Mirror mirror;
  mirror.set_name(name);
  mirror.destination.set_egressPort(getMirrorEgressPort(portName));
  mirror.set_dscp(dscp);
  mirror.set_truncate(truncate);
  return mirror;
}

cfg::GreTunnel getGreTunnelConfig(folly::IPAddress dstAddress) {
  cfg::GreTunnel greTunnel;
  greTunnel.set_ip(dstAddress.str());
  return greTunnel;
}

cfg::SflowTunnel getSflowTunnelConfig(
    folly::IPAddress dstAddress,
    uint16_t sPort,
    uint16_t dPort) {
  cfg::SflowTunnel sflowTunnel;
  sflowTunnel.set_ip(dstAddress.str());
  sflowTunnel.set_udpSrcPort(sPort);
  sflowTunnel.set_udpDstPort(dPort);
  return sflowTunnel;
}

cfg::MirrorTunnel getGREMirrorTunnelConfig(
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress) {
  cfg::MirrorTunnel mirrorTunnel;
  mirrorTunnel.set_greTunnel(getGreTunnelConfig(dstAddress));
  if (srcAddress) {
    mirrorTunnel.set_srcIp(srcAddress->str());
  }
  return mirrorTunnel;
}

cfg::MirrorTunnel getSflowMirrorTunnelConfig(
    folly::IPAddress dstAddress,
    uint16_t sPort,
    uint16_t dPort,
    std::optional<folly::IPAddress> srcAddress) {
  cfg::MirrorTunnel mirrorTunnel;
  mirrorTunnel.set_sflowTunnel(getSflowTunnelConfig(dstAddress, sPort, dPort));
  if (srcAddress) {
    mirrorTunnel.set_srcIp(srcAddress->str());
  }
  return mirrorTunnel;
}

cfg::Mirror getGREMirror(
    const std::string& name,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp,
    bool truncate) {
  cfg::Mirror mirror;
  mirror.set_name(name);
  mirror.set_dscp(dscp);
  mirror.set_truncate(truncate);
  mirror.destination.set_tunnel(
      getGREMirrorTunnelConfig(dstAddress, srcAddress));
  return mirror;
}

cfg::Mirror getGREMirrorWithPort(
    const std::string& name,
    PortID portID,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp,
    bool truncate) {
  cfg::Mirror mirror;
  mirror.set_name(name);
  mirror.set_dscp(dscp);
  mirror.set_truncate(truncate);
  mirror.destination.set_egressPort(getMirrorEgressPort(portID));
  mirror.destination.set_tunnel(
      getGREMirrorTunnelConfig(dstAddress, srcAddress));
  return mirror;
}

cfg::Mirror getGREMirrorWithPort(
    const std::string& name,
    const std::string& portName,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp,
    bool truncate) {
  cfg::Mirror mirror;
  mirror.set_name(name);
  mirror.set_dscp(dscp);
  mirror.set_truncate(truncate);
  mirror.destination.set_egressPort(getMirrorEgressPort(portName));
  mirror.destination.set_tunnel(
      getGREMirrorTunnelConfig(dstAddress, srcAddress));
  return mirror;
}

cfg::Mirror getSFlowMirror(
    const std::string& name,
    uint16_t sPort,
    uint16_t dPort,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp,
    bool truncate) {
  cfg::Mirror mirror;
  mirror.set_name(name);
  mirror.set_dscp(dscp);
  mirror.set_truncate(truncate);
  mirror.destination.set_tunnel(
      getSflowMirrorTunnelConfig(dstAddress, sPort, dPort, srcAddress));
  return mirror;
}

cfg::Mirror getSFlowMirrorWithPort(
    const std::string& name,
    PortID portID,
    uint16_t sPort,
    uint16_t dPort,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp,
    bool truncate

) {
  cfg::Mirror mirror;
  mirror.set_name(name);
  mirror.set_dscp(dscp);
  mirror.set_truncate(truncate);
  mirror.destination.set_egressPort(getMirrorEgressPort(portID));
  mirror.destination.set_tunnel(
      getSflowMirrorTunnelConfig(dstAddress, sPort, dPort, srcAddress));
  return mirror;
}

cfg::Mirror getSFlowMirrorWithPortName(
    const std::string& name,
    const std::string& portName,
    uint16_t sPort,
    uint16_t dPort,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp,
    bool truncate

) {
  cfg::Mirror mirror;
  mirror.set_name(name);
  mirror.set_dscp(dscp);
  mirror.set_truncate(truncate);
  mirror.destination.set_egressPort(getMirrorEgressPort(portName));
  mirror.destination.set_tunnel(
      getSflowMirrorTunnelConfig(dstAddress, sPort, dPort, srcAddress));
  return mirror;
}

} // namespace facebook::fboss::utility
