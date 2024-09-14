// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/state/PortDescriptor.h"

namespace facebook::fboss::utility {

namespace {

bool verifyResolvedLocalMirror(
    const SaiSwitch* saiSwitch,
    const state::MirrorFields& mirror,
    SaiMirrorHandle* mirrorHandle) {
  auto portHandle = saiSwitch->managerTable()->portManager().getPortHandle(
      PortID(*mirror.get_egressPort()));
  if (!portHandle) {
    return false;
  }
  auto monitorPort = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiLocalMirrorTraits::Attributes::MonitorPort());
  if (portHandle->port->adapterKey() != monitorPort) {
    return false;
  }

  auto type = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(), SaiLocalMirrorTraits::Attributes::Type());
  if (type != SAI_MIRROR_SESSION_TYPE_LOCAL) {
    return false;
  }
  return true;
}

bool verifyResolvedMirror(
    const SaiSwitch* saiSwitch,
    const state::MirrorFields& mirror,
    SaiMirrorHandle* mirrorHandle,
    sai_mirror_session_type_t session_type) {
  auto cfgPortDesc = mirror.get_egressPortDesc();
  auto egressPort =
      PortDescriptor::fromCfgCfgPortDescriptor(*cfgPortDesc).phyPortID();
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(egressPort);
  if (!portHandle) {
    return false;
  }
  auto monitorPort = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiEnhancedRemoteMirrorTraits::Attributes::MonitorPort());
  if (portHandle->port->adapterKey() != monitorPort) {
    return false;
  }

  auto type = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(), SaiLocalMirrorTraits::Attributes::Type());
  if (type != session_type) {
    return false;
  }

  // TODO: add truncate check

  // TOS
  auto tos = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiEnhancedRemoteMirrorTraits::Attributes::Tos());
  if (tos != mirror.get_dscp()) {
    return false;
  }

  const auto& tunnel = mirror.get_tunnel();
  if (!tunnel) {
    return false;
  }

  // src and dst IP
  auto srcIp = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiEnhancedRemoteMirrorTraits::Attributes::SrcIpAddress());
  if (srcIp != network::toIPAddress(tunnel->get_srcIp())) {
    return false;
  }
  auto dstIp = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiEnhancedRemoteMirrorTraits::Attributes::DstIpAddress());
  if (dstIp != network::toIPAddress(tunnel->get_dstIp())) {
    return false;
  }

  // src and dst MAC
  auto srcMac = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiEnhancedRemoteMirrorTraits::Attributes::SrcMacAddress());
  if (srcMac != folly::MacAddress(tunnel->get_srcMac())) {
    return false;
  }
  auto dstMac = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiEnhancedRemoteMirrorTraits::Attributes::DstMacAddress());
  if (dstMac != folly::MacAddress(tunnel->get_dstMac())) {
    return false;
  }

  // TTL
  auto ttl = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiEnhancedRemoteMirrorTraits::Attributes::Ttl());

  if (ttl != tunnel->get_ttl()) {
    return false;
  }
  return true;
}

bool verifyResolvedErspanMirror(
    const SaiSwitch* saiSwitch,
    const state::MirrorFields& mirror,
    SaiMirrorHandle* mirrorHandle) {
  return verifyResolvedMirror(
      saiSwitch, mirror, mirrorHandle, SAI_MIRROR_SESSION_TYPE_ENHANCED_REMOTE);
}

bool verifyResolvedSflowMirror(
    const SaiSwitch* saiSwitch,
    const state::MirrorFields& mirror,
    SaiMirrorHandle* mirrorHandle) {
  if (!verifyResolvedMirror(
          saiSwitch, mirror, mirrorHandle, SAI_MIRROR_SESSION_TYPE_SFLOW)) {
    return false;
  }

  // src and dst UDP port
  auto udpSrcPort = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiSflowMirrorTraits::Attributes::UdpSrcPort());

  auto srcPort = mirror.get_tunnel()->get_udpSrcPort();
  auto dstPort = mirror.get_tunnel()->get_udpDstPort();

  if (udpSrcPort != *srcPort) {
    return false;
  }
  auto udpDstPort = SaiApiTable::getInstance()->mirrorApi().getAttribute(
      mirrorHandle->adapterKey(),
      SaiSflowMirrorTraits::Attributes::UdpDstPort());
  if (udpDstPort != *dstPort) {
    return false;
  }
  return true;
}
} // namespace

bool HwTestThriftHandler::isMirrorProgrammed(
    std::unique_ptr<state::MirrorFields> mirror) {
  if (!mirror) {
    throw FbossError("isMirrorProgrammed: mirror is null");
  }
  if (!mirror->get_isResolved()) {
    XLOG(INFO) << "isMirrorProgrammed: " << mirror->get_name()
               << " is not resolved";
    return false;
  }
  XLOG(INFO) << "isMirrorProgrammed: " << mirror->get_name() << " is resolved";

  std::string jsonStr;
  apache::thrift::SimpleJSONSerializer::serialize(*mirror, &jsonStr);
  XLOG(INFO) << jsonStr;

  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  auto mirrorHandle =
      saiSwitch->managerTable()->mirrorManager().getMirrorHandle(
          mirror->get_name());
  if (!mirrorHandle) {
    return false;
  }
  auto tunnel = mirror->get_tunnel();
  if (!tunnel) {
    // regular local mirror
    return verifyResolvedLocalMirror(saiSwitch, *mirror, mirrorHandle);
  }
  if (tunnel->get_udpSrcPort()) {
    // sflow mirror
    return verifyResolvedSflowMirror(saiSwitch, *mirror, mirrorHandle);
  }
  // erspan mirror
  return verifyResolvedErspanMirror(saiSwitch, *mirror, mirrorHandle);
}

bool HwTestThriftHandler::isPortMirrored(
    int32_t port,
    std::unique_ptr<std::string> mirror,
    bool ingress) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(PortID(port));
  std::optional<SaiPortTraits::Attributes::IngressMirrorSession> ingressMirror;
  std::optional<SaiPortTraits::Attributes::EgressMirrorSession> egressMirror;

  std::optional<sai_object_id_t> mirrorSaiOid{};
  if (ingress) {
    ingressMirror = SaiApiTable::getInstance()->portApi().getAttribute(
        portHandle->port->adapterKey(), ingressMirror);
  } else {
    egressMirror = SaiApiTable::getInstance()->portApi().getAttribute(
        portHandle->port->adapterKey(), egressMirror);
  }
  if (!ingressMirror && !egressMirror) {
    XLOG(DBG2) << "Port " << port << " is not mirrored";
    return false;
  } else if (ingressMirror) {
    mirrorSaiOid = ingressMirror.value().value()[0];
  } else if (egressMirror) {
    mirrorSaiOid = egressMirror.value().value()[0];
  }
  if (!mirrorSaiOid) {
    return false;
  }

  auto mirrorHandle =
      saiSwitch->managerTable()->mirrorManager().getMirrorHandle(*mirror);
  if (!mirrorHandle) {
    XLOG(DBG2) << "Mirror " << *mirror << " not found";
    return false;
  }

  return mirrorHandle->adapterKey() == mirrorSaiOid.value();
}

bool HwTestThriftHandler::isPortSampled(
    int32_t /*port*/,
    std::unique_ptr<std::string> /*mirror*/,
    bool /*ingress*/) {
  throw FbossError("isPortSampled not implemented");
}

bool HwTestThriftHandler::isAclEntryMirrored(
    std::unique_ptr<std::string> /*aclEntry*/,
    std::unique_ptr<std::string> /*mirror*/,
    bool /*ingress*/) {
  throw FbossError("isAclEntryMirrored not implemented");
}
} // namespace facebook::fboss::utility
