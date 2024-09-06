// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmMirror.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmMirrorUtils.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Mirror.h"

#include <boost/container/flat_map.hpp>

#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>
#include <optional>

namespace {
int getMirrorFlags(const std::optional<facebook::fboss::MirrorTunnel>& tunnel) {
  if (!tunnel) {
    return 0;
  }
  if (tunnel->udpPorts) {
    return BCM_MIRROR_DEST_TUNNEL_SFLOW;
  }
  return BCM_MIRROR_DEST_TUNNEL_IP_GRE;
}
} // namespace
namespace facebook::fboss {

BcmMirrorDestination::BcmMirrorDestination(
    int unit,
    BcmPort* egressPort,
    uint8_t dscp,
    bool truncate)
    : unit_(unit) {
  bcm_mirror_destination_t mirror_destination;
  bcm_mirror_destination_t_init(&mirror_destination);
  mirror_destination.gport = egressPort->getBcmGport();
  mirror_destination.tos = dscp;
  if (truncate) {
    mirror_destination.truncate = BCM_MIRROR_PAYLOAD_TRUNCATE;
  }
  auto rv = bcm_mirror_destination_create(unit_, &mirror_destination);
  bcmCheckError(rv, "failed to create mirror on port");
  handle_ = mirror_destination.mirror_dest_id;
  flags_ = mirror_destination.flags;
}

BcmMirrorDestination::BcmMirrorDestination(
    int unit,
    BcmPort* egressPort,
    const MirrorTunnel& mirrorTunnel,
    uint8_t dscp,
    bool truncate)
    : unit_(unit) {
  bcm_mirror_destination_t mirror_destination;
  bcm_mirror_destination_t_init(&mirror_destination);
  mirror_destination.gport = egressPort->getBcmGport();
  if (mirrorTunnel.udpPorts.has_value()) {
    mirror_destination.flags = BCM_MIRROR_DEST_TUNNEL_SFLOW;
    mirror_destination.udp_src_port = mirrorTunnel.udpPorts.value().udpSrcPort;
    mirror_destination.udp_dst_port = mirrorTunnel.udpPorts.value().udpDstPort;
    XLOG(DBG2) << "SFLOW tunnel: source([" << mirrorTunnel.srcIp
               << "]:" << mirror_destination.udp_src_port << "/"
               << mirrorTunnel.srcMac << "), destination(["
               << mirrorTunnel.dstIp << "]:" << mirror_destination.udp_dst_port
               << "/" << mirrorTunnel.dstMac << ")";

  } else {
    mirror_destination.gre_protocol = mirrorTunnel.greProtocol;
    mirror_destination.flags = BCM_MIRROR_DEST_TUNNEL_IP_GRE;
    XLOG(DBG2) << "ERSPAN(I) tunnel: source(" << mirrorTunnel.srcIp << "/"
               << mirrorTunnel.srcMac << "), destination(" << mirrorTunnel.dstIp
               << "/" << mirrorTunnel.dstMac << ")";
  }
  mirror_destination.ttl = mirrorTunnel.ttl;
  mirror_destination.tos = dscp;
  if (truncate) {
    mirror_destination.truncate = BCM_MIRROR_PAYLOAD_TRUNCATE;
  }
  macToBcm(mirrorTunnel.srcMac, &mirror_destination.src_mac);
  macToBcm(mirrorTunnel.dstMac, &mirror_destination.dst_mac);

  if (mirrorTunnel.dstIp.isV4()) {
    mirror_destination.version = 4;

    mirror_destination.src_addr = mirrorTunnel.srcIp.asV4().toLongHBO();
    mirror_destination.dst_addr = mirrorTunnel.dstIp.asV4().toLongHBO();
  } else {
    mirror_destination.version = 6;
    ipToBcmIp6(mirrorTunnel.srcIp, &mirror_destination.src6_addr);
    ipToBcmIp6(mirrorTunnel.dstIp, &mirror_destination.dst6_addr);
  }
  auto rv = bcm_mirror_destination_create(unit_, &mirror_destination);
  bcmCheckError(rv, "failed to create mirror to ", mirrorTunnel.dstIp.str());
  handle_ = mirror_destination.mirror_dest_id;
  flags_ = mirror_destination.flags;
}

BcmMirrorHandle BcmMirrorDestination::getHandle() {
  return handle_;
}

BcmMirrorDestination::~BcmMirrorDestination() {
  auto rv = bcm_mirror_destination_destroy(unit_, handle_);
  bcmCheckError(rv, "failed to remove mirror ");
}

BcmMirror::BcmMirror(BcmSwitch* hw, const std::shared_ptr<Mirror>& mirror)
    : hw_(hw), mirrorName_(mirror->getID()) {
  program(mirror);
  // TODO(pshaikh) : if the port action apply fails for any of the port and
  // exception is thrown, mirror action stop should be applied.
  // To achieve this, maintain list of ports/acls mirrored and unmirrored and in
  // destructor use it. in MirrorAction::START, fill up that list in
  // MirrorAction::STOP, clear that list
  applyPortMirrorActions(MirrorAction::START);
  applyAclMirrorActions(MirrorAction::START);
}

BcmMirror::~BcmMirror() {
  // before unmirroring acls or ports, check if acl/port table are not deleted
  // this is required because of the order of table deletions, mirror table
  // is deleted after acl and port tables, but this is still needed to consider
  // the case of mirror update either due to resolution update or config update.
  if (hw_->getPortTable()) {
    applyPortMirrorActions(MirrorAction::STOP);
  }
  if (hw_->getAclTable()) {
    applyAclMirrorActions(MirrorAction::STOP);
  }
  XLOG(DBG3) << "destroyed mirror " << mirrorName_;
}

void BcmMirror::program(const std::shared_ptr<Mirror>& mirror) {
  if (!mirror->isResolved()) {
    return;
  }

  CHECK(!destination_);
  auto egressPort = mirror->getEgressPortDesc().has_value()
      ? mirror->getEgressPortDesc().value().phyPortID()
      : mirror->getEgressPort().value();
  auto* bcmPort = hw_->getPortTable()->getBcmPortIf(egressPort);
  auto* warmBootCache = hw_->getWarmBootCache();
  auto iter = warmBootCache->findMirror(
      bcmPort->getBcmGport(), mirror->getMirrorTunnel());
  if (iter != warmBootCache->mirrorsEnd()) {
    destination_ = std::make_unique<BcmMirrorDestination>(
        hw_->getUnit(),
        iter->second,
        getMirrorFlags(mirror->getMirrorTunnel()));
    warmBootCache->programmedMirror(iter);
    XLOG(DBG3) << "found mirror " << mirrorName_
               << " with handle: " << destination_->getHandle()
               << " in warmboot cache";
    return;
  }
  bool truncate = mirror->getTruncate();
  if (truncate) {
    CHECK(hw_->getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::MIRROR_PACKET_TRUNCATION))
        << "Mirrored packet truncation is not supported on this platform";
  }

  if (mirror->getMirrorTunnel().has_value()) {
    destination_ = std::make_unique<BcmMirrorDestination>(
        hw_->getUnit(),
        bcmPort,
        mirror->getMirrorTunnel().value(),
        mirror->getDscp(),
        truncate);
  } else {
    destination_ = std::make_unique<BcmMirrorDestination>(
        hw_->getUnit(), bcmPort, mirror->getDscp(), truncate);
  }
  XLOG(DBG3) << "created mirror " << mirrorName_
             << " with handle: " << destination_->getHandle();
}

void BcmMirror::applyPortMirrorAction(
    PortID port,
    MirrorAction action,
    MirrorDirection direction,
    cfg::SampleDestination sampleDest) {
  if (!isProgrammed()) {
    return;
  }
  uint32_t flags = directionToBcmPortMirrorFlag(direction) |
      sampleDestinationToBcmPortMirrorSflowFlag(sampleDest);
  auto handle = destination_->getHandle();
  switch (action) {
    case MirrorAction::START: {
      auto* warmBootCache = hw_->getWarmBootCache();
      auto* bcmPort = hw_->getPortTable()->getBcmPort(port);
      auto iter =
          warmBootCache->findMirroredPort(bcmPort->getBcmGport(), flags);
      if (iter != warmBootCache->mirroredPortsEnd()) {
        CHECK_EQ(handle, iter->second);
        warmBootCache->programmedMirroredPort(iter);
      } else {
        /* ports are not associated individually with sflow mirror for sampling.
         * doing so would cause _E_EXISTS error. ASIC behaves differently for
         * sflow sampled ports. Once first port-mirror mapping for one port is
         * established, it is also established for all other ports. to avoid
         * this unusual behavior, mirror-port association is done only for very
         * first port. */
        if (destination_->getFlags() & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
          if (mirroredPorts_ > 0) {
            XLOG(DBG3) << "skipped " << mirrorDirectionName(direction)
                       << " sflow mirror: " << mirrorName_
                       << " on port: " << port;
          } else {
            /* add sflow mirror for obly the first port. this will activate
             * sflow mirror for all ports */
            bcmCheckError(
                bcm_mirror_port_dest_add(
                    hw_->getUnit(), BCM_PORT_INVALID, flags, handle),
                "failed to add port mapping for sflow mirror");
          }
        } else {
          bcmCheckError(
              bcm_mirror_port_dest_add(
                  hw_->getUnit(), static_cast<int>(port), flags, handle),
              "failed to add port ",
              port,
              " to ",
              mirrorDirectionName(direction),
              " mirror ",
              mirrorName_);
          XLOG(DBG3) << "added " << mirrorDirectionName(direction)
                     << " mirror: " << mirrorName_ << " on port: " << port;
        }
      }
      mirroredPorts_++;
    } break;
    case MirrorAction::STOP: {
      if (destination_->getFlags() & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
        /*  since ports are not associated individually with sflow mirror for
         * sampling as doing so cause _E_EXISTS error(described above).
         * accordingly, mirror-port association is not removed individually.
         * instead a mirror-port mapping is removed only when no port refers
         * this mirror. this also deletes mapping for all other ports
         * referencing the mirror */
        if (mirroredPorts_ > 1) {
          XLOG(DBG3) << "skipped " << mirrorDirectionName(direction)
                     << " sflow mirror: " << mirrorName_
                     << " on port: " << port;
        } else if (mirroredPorts_ == 1) {
          /* remove sflow mirror for the last port. this will will deactivate
           * for sflow mirror for all port */
          bcmCheckError(
              bcm_mirror_port_dest_delete(
                  hw_->getUnit(), BCM_PORT_INVALID, flags, handle),
              "failed to delete port mapping for sflow mirror");
        }
      } else {
        bcmCheckError(
            bcm_mirror_port_dest_delete(
                hw_->getUnit(), static_cast<int>(port), flags, handle),
            "failed to delete port ",
            port,
            " from ",
            mirrorDirectionName(direction),
            " mirror ",
            mirrorName_);
        XLOG(DBG3) << "removed " << mirrorDirectionName(direction)
                   << " mirror: " << mirrorName_ << " on port: " << port;
      }
      mirroredPorts_--;
    } break;
  }
}

void BcmMirror::applyAclMirrorAction(
    BcmAclEntryHandle aclEntryHandle,
    MirrorAction action,
    MirrorDirection direction) {
  if (!isProgrammed()) {
    return;
  }

  auto handle = destination_->getHandle();
  switch (action) {
    case MirrorAction::START: {
      auto* warmBootCache = hw_->getWarmBootCache();
      auto iter = warmBootCache->findMirroredAcl(aclEntryHandle, direction);
      if (iter != warmBootCache->mirroredAclsEnd()) {
        CHECK_EQ(handle, iter->second);
        warmBootCache->programmedMirroredAcl(iter);
        return;
      } else {
        bcmCheckError(
            bcm_field_action_add(
                hw_->getUnit(),
                aclEntryHandle,
                directionToBcmAclMirrorAction(direction),
                0,
                handle),
            "failed to add acl ",
            aclEntryHandle,
            " to ",
            mirrorDirectionName(direction),
            " mirror ",
            mirrorName_);
      }
      XLOG(DBG3) << "added " << mirrorDirectionName(direction)
                 << " mirror: " << mirrorName_ << " on acl: " << aclEntryHandle;

    } break;
    case MirrorAction::STOP:
      bcmCheckError(
          bcm_field_action_delete(
              hw_->getUnit(),
              aclEntryHandle,
              directionToBcmAclMirrorAction(direction),
              0,
              handle),
          "failed to delete acl ",
          aclEntryHandle,
          " from ",
          mirrorDirectionName(direction),
          " mirror ",
          mirrorName_);
      XLOG(DBG3) << "removed " << mirrorDirectionName(direction)
                 << " mirror: " << mirrorName_ << " on acl: " << aclEntryHandle;

      break;
  }
  auto rv = bcm_field_entry_install(hw_->getUnit(), aclEntryHandle);
  bcmCheckError(rv, "failed to reload acl ", aclEntryHandle);
}

void BcmMirror::applyPortMirrorActions(MirrorAction action) {
  for (const auto& [portID, bcmPort] : *hw_->getPortTable()) {
    if (bcmPort->getIngressPortMirror() == mirrorName_) {
      applyPortMirrorAction(
          portID,
          action,
          MirrorDirection::INGRESS,
          bcmPort->getSampleDestination());
    }
    if (bcmPort->getEgressPortMirror() == mirrorName_) {
      applyPortMirrorAction(
          portID,
          action,
          MirrorDirection::EGRESS,
          bcmPort->getSampleDestination());
    }
  }
}

void BcmMirror::applyAclMirrorActions(MirrorAction action) {
  hw_->getAclTable()->forFilteredEach(
      [=](const auto& aclTableEntry) {
        const auto& bcmAclEntry = aclTableEntry.second;
        return bcmAclEntry->getIngressAclMirror() == mirrorName_ ||
            bcmAclEntry->getEgressAclMirror() == mirrorName_;
      },
      [=](const auto& aclTableEntry) {
        const auto& bcmAclEntry = aclTableEntry.second;
        const auto bcmAclEntryHandle = bcmAclEntry->getHandle();

        if (bcmAclEntry->getIngressAclMirror() == mirrorName_) {
          applyAclMirrorAction(
              bcmAclEntryHandle, action, MirrorDirection::INGRESS);
        }
        if (bcmAclEntry->getEgressAclMirror() == mirrorName_) {
          applyAclMirrorAction(
              bcmAclEntryHandle, action, MirrorDirection::EGRESS);
        }
      });
}

} // namespace facebook::fboss
