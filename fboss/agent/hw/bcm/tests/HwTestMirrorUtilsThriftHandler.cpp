// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/bcm/BcmMirror.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"

namespace facebook::fboss::utility {

using MirrorTraverseArgT = std::pair<bcm_mirror_destination_t*, int>;

static int traverse(
    int /* unit */,
    bcm_mirror_destination_t* mirror_dest,
    void* user_data) {
  MirrorTraverseArgT* args = static_cast<MirrorTraverseArgT*>(user_data);
  bcm_mirror_destination_t* buffer = args->first;
  int handle = args->second;
  if (mirror_dest->mirror_dest_id == handle) {
    std::memcpy(buffer, mirror_dest, sizeof(bcm_mirror_destination_t));
  }
  return 0;
}

bool HwTestThriftHandler::isMirrorProgrammed(
    std::unique_ptr<state::MirrorFields> mirror) {
  if (!mirror) {
    throw FbossError("isMirrorProgrammed: mirror is null");
  }
  if (!folly::copy(mirror->isResolved().value())) {
    throw FbossError("isMirrorProgrammed: mirror is not resolved");
  }

  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch_);
  const auto* bcmMirrorTable = bcmSwitch->getBcmMirrorTable();
  auto* bcmMirror = bcmMirrorTable->getNodeIf(mirror->name().value());
  if (!bcmMirror || !bcmMirror->isProgrammed()) {
    return false;
  }

  auto handle = bcmMirror->getHandle().value();
  bcm_mirror_destination_t mirror_dest;
  bcm_mirror_destination_t_init(&mirror_dest);
  MirrorTraverseArgT args = std::make_pair(&mirror_dest, handle);

  bcm_mirror_destination_traverse(0, &traverse, &args);
  if (mirror_dest.mirror_dest_id != handle) {
    return false;
  }

  bcm_gport_t gport;
  if (auto cfgPortDesc = mirror->egressPortDesc()) {
    auto egressPort =
        PortDescriptor::fromCfgCfgPortDescriptor(*cfgPortDesc).phyPortID();
    BCM_GPORT_MODPORT_SET(gport, bcmSwitch->getUnit(), egressPort);
    if (mirror_dest.gport != gport) {
      return false;
    }
  } else {
    return false;
  }

  if (mirror_dest.tos != folly::copy(mirror->dscp().value())) {
    return false;
  }
  if (bool(mirror_dest.truncate & BCM_MIRROR_PAYLOAD_TRUNCATE) !=
      folly::copy(mirror->truncate().value())) {
    return false;
  }
  auto tunnel = apache::thrift::get_pointer(mirror->tunnel());
  if (!tunnel) {
    if (mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
      return false;
    }
    if (mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE) {
      return false;
    }
    return true;
  }

  auto dstPort = apache::thrift::get_pointer(tunnel->udpDstPort());
  auto srcPort = apache::thrift::get_pointer(tunnel->udpSrcPort());
  if (mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
    if (!srcPort || !dstPort) {
      return false;
    }
    if (mirror_dest.udp_src_port != *srcPort ||
        mirror_dest.udp_dst_port != *dstPort) {
      return false;
    }
  } else if (!(mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE)) {
    return false;
  }

  auto srcIp = network::toIPAddress(tunnel->srcIp().value());
  auto dstIp = network::toIPAddress(tunnel->dstIp().value());
  if (mirror_dest.version == 4) {
    if (!srcIp.isV4() || !dstIp.isV4()) {
      return false;
    }
    if (srcIp != folly::IPAddress::fromLongHBO(mirror_dest.src_addr) ||
        dstIp != folly::IPAddress::fromLongHBO(mirror_dest.dst_addr)) {
      return false;
    }
  } else if (mirror_dest.version == 6) {
    if (!srcIp.isV6() || !dstIp.isV6()) {
      return false;
    }
    if (srcIp !=
            folly::IPAddress::fromBinary(
                folly::ByteRange(mirror_dest.src6_addr, 16)) ||
        dstIp !=
            folly::IPAddress::fromBinary(
                folly::ByteRange(mirror_dest.dst6_addr, 16))) {
      return false;
    }
  }

  if (folly::MacAddress(tunnel->srcMac().value()) !=
          macFromBcm(mirror_dest.src_mac) ||
      folly::MacAddress(tunnel->dstMac().value()) !=
          macFromBcm(mirror_dest.dst_mac)) {
    return false;
  }
  if (folly::copy(tunnel->ttl().value()) != mirror_dest.ttl) {
    return false;
  }
  return true;
}

bool HwTestThriftHandler::isPortMirrored(
    int32_t port,
    std::unique_ptr<std::string> mirror,
    bool ingress) {
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch_);
  bcm_gport_t port_mirror_dest_id = 0;
  int count = 0;
  bcm_mirror_port_dest_get(
      bcmSwitch->getUnit(),
      port,
      ingress ? BCM_MIRROR_PORT_INGRESS : BCM_MIRROR_PORT_EGRESS,
      1,
      &port_mirror_dest_id,
      &count);

  if (count == 0) {
    return false;
  }

  const auto* bcmMirrorTable = bcmSwitch->getBcmMirrorTable();
  auto* bcmMirror = bcmMirrorTable->getNodeIf(*mirror);

  if (!bcmMirror) {
    return false;
  }
  return (port_mirror_dest_id == bcmMirror->getHandle());
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
