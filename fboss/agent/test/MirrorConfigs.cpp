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
  mirror.name_ref() = name;
  mirror.destination_ref()->egressPort_ref().emplace(
      getMirrorEgressPort(portID));
  mirror.dscp_ref() = dscp;
  mirror.truncate_ref() = truncate;
  return mirror;
}

cfg::Mirror getSPANMirror(
    const std::string& name,
    const std::string& portName,
    uint8_t dscp,
    bool truncate) {
  cfg::Mirror mirror;
  mirror.name_ref() = name;
  mirror.destination_ref()->egressPort_ref().emplace(
      getMirrorEgressPort(portName));
  mirror.dscp_ref() = dscp;
  mirror.truncate_ref() = truncate;
  return mirror;
}

cfg::GreTunnel getGreTunnelConfig(folly::IPAddress dstAddress) {
  cfg::GreTunnel greTunnel;
  greTunnel.ip_ref() = dstAddress.str();
  return greTunnel;
}

cfg::SflowTunnel getSflowTunnelConfig(
    folly::IPAddress dstAddress,
    uint16_t sPort,
    uint16_t dPort) {
  cfg::SflowTunnel sflowTunnel;
  sflowTunnel.ip_ref() = dstAddress.str();
  sflowTunnel.udpSrcPort_ref() = sPort;
  sflowTunnel.udpDstPort_ref() = dPort;
  return sflowTunnel;
}

cfg::MirrorTunnel getGREMirrorTunnelConfig(
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress) {
  cfg::MirrorTunnel mirrorTunnel;
  mirrorTunnel.greTunnel_ref().emplace(getGreTunnelConfig(dstAddress));
  if (srcAddress) {
    mirrorTunnel.srcIp_ref().emplace(srcAddress->str());
  }
  return mirrorTunnel;
}

cfg::MirrorTunnel getSflowMirrorTunnelConfig(
    folly::IPAddress dstAddress,
    uint16_t sPort,
    uint16_t dPort,
    std::optional<folly::IPAddress> srcAddress) {
  cfg::MirrorTunnel mirrorTunnel;
  mirrorTunnel.sflowTunnel_ref().emplace(
      getSflowTunnelConfig(dstAddress, sPort, dPort));
  if (srcAddress) {
    mirrorTunnel.srcIp_ref().emplace(srcAddress->str());
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
  mirror.name_ref() = name;
  mirror.dscp_ref() = dscp;
  mirror.truncate_ref() = truncate;
  mirror.destination_ref()->tunnel_ref().emplace(
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
  mirror.name_ref() = name;
  mirror.dscp_ref() = dscp;
  mirror.truncate_ref() = truncate;
  mirror.destination_ref()->egressPort_ref().emplace(
      getMirrorEgressPort(portID));
  mirror.destination_ref()->tunnel_ref().emplace(
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
  mirror.name_ref() = name;
  mirror.dscp_ref() = dscp;
  mirror.truncate_ref() = truncate;
  mirror.destination_ref()->egressPort_ref().emplace(
      getMirrorEgressPort(portName));
  mirror.destination_ref()->tunnel_ref().emplace(
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
  mirror.name_ref() = name;
  mirror.dscp_ref() = dscp;
  mirror.truncate_ref() = truncate;
  mirror.destination_ref()->tunnel_ref().emplace(
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
  mirror.name_ref() = name;
  mirror.dscp_ref() = dscp;
  mirror.truncate_ref() = truncate;
  mirror.destination_ref()->egressPort_ref().emplace(
      getMirrorEgressPort(portID));
  mirror.destination_ref()->tunnel_ref().emplace(
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
  mirror.name_ref() = name;
  mirror.dscp_ref() = dscp;
  mirror.truncate_ref() = truncate;
  mirror.destination_ref()->egressPort_ref().emplace(
      getMirrorEgressPort(portName));
  mirror.destination_ref()->tunnel_ref().emplace(
      getSflowMirrorTunnelConfig(dstAddress, sPort, dPort, srcAddress));
  return mirror;
}

} // namespace facebook::fboss::utility
