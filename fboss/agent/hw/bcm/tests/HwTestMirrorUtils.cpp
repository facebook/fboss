/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestMirrorUtils.h"

#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/state/Mirror.h"

#include <gtest/gtest.h>

extern "C" {
#include <bcm/field.h>
#include <bcm/mirror.h>
}

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

void getAllMirrorDestinations(
    facebook::fboss::HwSwitch* hwSwitch,
    std::vector<uint64_t>& destinations) {
  auto traverse = [&destinations](bcm_mirror_destination_t* destination) {
    destinations.push_back(destination->mirror_dest_id);
  };

  auto callback = [](int /*unit*/,
                     bcm_mirror_destination_t* destination,
                     void* closure) -> int {
    (*static_cast<decltype(traverse)*>(closure))(destination);
    return 0;
  };
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  bcm_mirror_destination_traverse(bcmSwitch->getUnit(), callback, &traverse);
}

uint32_t getMirrorPortIngressAndSflowFlags() {
  return BCM_MIRROR_PORT_INGRESS | BCM_MIRROR_PORT_SFLOW;
}

uint32_t getMirrorPortIngressFlags() {
  return BCM_MIRROR_PORT_INGRESS;
}

uint32_t getMirrorPortEgressFlags() {
  return BCM_MIRROR_PORT_EGRESS;
}

uint32_t getMirrorPortSflowFlags() {
  return BCM_MIRROR_PORT_SFLOW;
}

void verifyResolvedMirror(
    facebook::fboss::HwSwitch* hwSwitch,
    const std::shared_ptr<facebook::fboss::Mirror>& mirror) {
  ASSERT_NE(mirror, nullptr);
  ASSERT_EQ(mirror->isResolved(), true);
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  const auto* bcmMirrorTable = bcmSwitch->getBcmMirrorTable();
  auto* bcmMirror = bcmMirrorTable->getNodeIf(mirror->getID());
  ASSERT_NE(bcmMirror, nullptr);
  ASSERT_TRUE(bcmMirror->isProgrammed());

  auto handle = bcmMirror->getHandle().value();
  bcm_mirror_destination_t mirror_dest;
  bcm_mirror_destination_t_init(&mirror_dest);
  MirrorTraverseArgT args = std::make_pair(&mirror_dest, handle);

  bcm_mirror_destination_traverse(0, &traverse, &args);

  EXPECT_EQ(mirror_dest.mirror_dest_id, handle);
  bcm_gport_t gport;
  BCM_GPORT_MODPORT_SET(
      gport, bcmSwitch->getUnit(), mirror->getEgressPort().value());
  EXPECT_EQ(mirror_dest.gport, gport);
  EXPECT_EQ(mirror_dest.tos, mirror->getDscp());
  EXPECT_EQ(
      bool(mirror_dest.truncate & BCM_MIRROR_PAYLOAD_TRUNCATE),
      mirror->getTruncate());
  if (!mirror->getMirrorTunnel().has_value()) {
    return;
  }
  const auto& tunnel = mirror->getMirrorTunnel();
  std::optional<TunnelUdpPorts> udpPorts = mirror->getTunnelUdpPorts();
  EXPECT_EQ(
      udpPorts.has_value(),
      bool(mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW));
  EXPECT_EQ(
      !udpPorts.has_value(),
      bool(mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE));

  if (udpPorts.has_value()) {
    EXPECT_EQ(udpPorts.value().udpSrcPort, mirror_dest.udp_src_port);
    EXPECT_EQ(udpPorts.value().udpDstPort, mirror_dest.udp_dst_port);
    EXPECT_NE(0, mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW);
  } else {
    EXPECT_EQ(tunnel->greProtocol, mirror_dest.gre_protocol);
    EXPECT_NE(0, mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE);
  }
  if (mirror->getDestinationIp()->isV4()) {
    EXPECT_EQ(mirror_dest.version, 4);
    EXPECT_EQ(
        tunnel->srcIp, folly::IPAddress::fromLongHBO(mirror_dest.src_addr));
    EXPECT_EQ(
        tunnel->dstIp, folly::IPAddress::fromLongHBO(mirror_dest.dst_addr));
  } else {
    EXPECT_EQ(mirror_dest.version, 6);
    EXPECT_EQ(
        tunnel->srcIp,
        folly::IPAddress::fromBinary(
            folly::ByteRange(mirror_dest.src6_addr, 16)));
    EXPECT_EQ(
        tunnel->dstIp,
        folly::IPAddress::fromBinary(
            folly::ByteRange(mirror_dest.dst6_addr, 16)));
  }
  EXPECT_EQ(tunnel->srcMac, macFromBcm(mirror_dest.src_mac));
  EXPECT_EQ(tunnel->dstMac, macFromBcm(mirror_dest.dst_mac));
  EXPECT_EQ(tunnel->ttl, mirror_dest.ttl);
}

void verifyUnResolvedMirror(
    facebook::fboss::HwSwitch* hwSwitch,
    const std::shared_ptr<facebook::fboss::Mirror>& mirror) {
  ASSERT_NE(mirror, nullptr);
  ASSERT_EQ(mirror->isResolved(), false);
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  const auto* bcmMirrorTable = bcmSwitch->getBcmMirrorTable();
  auto* bcmMirror = bcmMirrorTable->getNodeIf(mirror->getID());
  ASSERT_NE(bcmMirror, nullptr);
  ASSERT_FALSE(bcmMirror->isProgrammed());
}

void verifyPortMirrorDestination(
    facebook::fboss::HwSwitch* hwSwitch,
    PortID port,
    uint32_t flags,
    uint64_t mirror_dest_id) {
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  bcm_gport_t port_mirror_dest_id = 0;
  int count = 0;
  bcm_mirror_port_dest_get(
      bcmSwitch->getUnit(), port, flags, 1, &port_mirror_dest_id, &count);

  EXPECT_NE(count, 0);
  EXPECT_EQ(port_mirror_dest_id, mirror_dest_id);
}

void verifyAclMirrorDestination(
    facebook::fboss::HwSwitch* hwSwitch,
    const std::string& mirrorId) {
  std::vector<uint64_t> destinations;
  getAllMirrorDestinations(hwSwitch, destinations);
  ASSERT_EQ(destinations.size(), 1);
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  bcmSwitch->getAclTable()->forFilteredEach(
      [=](const auto& pair) {
        const auto& bcmAclEntry = pair.second;
        return (bcmAclEntry->getIngressAclMirror() == mirrorId) ||
            (bcmAclEntry->getEgressAclMirror() == mirrorId);
      },
      [&](const auto& pair) {
        const auto& bcmAclEntry = pair.second;
        auto bcmAclHandle = bcmAclEntry->getHandle();
        uint32_t param0 = 0;
        uint32_t param1 = 0;
        bcm_field_action_get(
            0, bcmAclHandle, bcmFieldActionMirrorIngress, &param0, &param1);
        EXPECT_EQ(param1, (bcm_gport_t)destinations[0]);
        param0 = 0, param1 = 0;
        bcm_field_action_get(
            0, bcmAclHandle, bcmFieldActionMirrorEgress, &param0, &param1);
        EXPECT_EQ(param1, (bcm_gport_t)destinations[0]);
      });
}

void verifyNoAclMirrorDestination(
    facebook::fboss::HwSwitch* hwSwitch,
    const std::string& /* mirrorId */) {
  std::vector<uint64_t> destinations;
  getAllMirrorDestinations(hwSwitch, destinations);
  ASSERT_EQ(destinations.size(), 1);
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  bcmSwitch->getAclTable()->forFilteredEach(
      [=](const auto& /*pair*/) {
        return true; /* check for all acls */
      },
      [&](const auto& pair) {
        const auto& bcmAclEntry = pair.second;
        auto bcmAclHandle = bcmAclEntry->getHandle();
        uint32_t param0 = 0;
        uint32_t param1 = 0;
        bcm_field_action_get(
            bcmSwitch->getUnit(),
            bcmAclHandle,
            bcmFieldActionMirrorIngress,
            &param0,
            &param1);
        EXPECT_NE(param1, destinations[0]);
        param0 = 0, param1 = 0;
        bcm_field_action_get(
            bcmSwitch->getUnit(),
            bcmAclHandle,
            bcmFieldActionMirrorEgress,
            &param0,
            &param1);
        EXPECT_NE(param1, destinations[0]);
      });
}

void verifyPortNoMirrorDestination(
    facebook::fboss::HwSwitch* hwSwitch,
    PortID port,
    uint32_t flags) {
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  bcm_gport_t port_mirror_dest_id = 0;
  int count = 0;
  bcm_mirror_port_dest_get(
      bcmSwitch->getUnit(), port, flags, 1, &port_mirror_dest_id, &count);

  EXPECT_EQ(count, 0);
  EXPECT_EQ(port_mirror_dest_id, 0);
}

bool isMirrorSflowTunnelEnabled(
    facebook::fboss::HwSwitch* hwSwitch,
    uint64_t destination) {
  bcm_mirror_destination_t mirror_dest;
  bcm_mirror_destination_t_init(&mirror_dest);
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  bcm_mirror_destination_get(bcmSwitch->getUnit(), destination, &mirror_dest);
  return mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW;
}

HwResourceStats getHwTableStats(HwSwitch* hwSwitch) {
  BcmSwitch* bcmSwitch = static_cast<BcmSwitch*>(hwSwitch);
  return bcmSwitch->getStatUpdater()->getHwTableStats();
}

} // namespace facebook::fboss::utility
