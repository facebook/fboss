/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmRxPacket.h"
#include "fboss/agent/hw/bcm/gen-cpp2/packettrace_types.h"

#include <folly/Memory.h>
#include <folly/logging/xlog.h>

extern "C" {
#include <opennsl/l2.h>
#include <opennsl/link.h>
#include <opennsl/error.h>
}

/*
 * Stubbed out.
 */
namespace facebook { namespace fboss {

std::unique_ptr<BcmRxPacket> BcmSwitch::createRxPacket(opennsl_pkt_t* pkt) {
  return std::make_unique<BcmRxPacket>(pkt);
}

bool BcmSwitch::isAlpmEnabled() {
  return false;
}

void BcmSwitch::dropDhcpPackets() {}

void BcmSwitch::setL3MtuFailPackets() {}

void BcmSwitch::setupCos() {}

void BcmSwitch::setupFPGroups() {}

void BcmSwitch::initMirrorModule() const {}

bool BcmSwitch::haveMissingOrQSetChangedFPGroups() const {
  return false;
}

void BcmSwitch::copyIPv6LinkLocalMcastPackets() {
  // OpenNSL doesn't yet provide functions for adding field-processor rules
  // for capturing packets
}

void BcmSwitch::configureRxRateLimiting() {
  // OpenNSL doesn't yet provide functions for configuring rate-limiting,
  // so rate limiting settings must be baked into the binary driver.
}

bool BcmSwitch::isRxThreadRunning() {
  // FIXME(orib): Right now, the BCM calls to figure out if rx is active are not
  // exported. Since initializing the driver sets up the rx thread, it should
  // be safe to just return true here.
  return true;
}

bool BcmSwitch::handleSflowPacket(opennsl_pkt_t* /* pkt */) noexcept {
  return false;
}

std::string BcmSwitch::gatherSdkState() const {
  return "";
}

void BcmSwitch::stopLinkscanThread() {
    // Disable the linkscan thread
  auto rv = opennsl_linkscan_enable_set(unit_, 0);
  CHECK(OPENNSL_SUCCESS(rv)) << "failed to stop BcmSwitch linkscan thread "
                             << opennsl_errmsg(rv);
}

std::unique_ptr<PacketTraceInfo> BcmSwitch::getPacketTrace(
    std::unique_ptr<MockRxPacket> /*pkt*/) {
  return nullptr;
}

void BcmSwitch::exportSdkVersion() const {}

void BcmSwitch::initFieldProcessor() const {
  // API not available in opennsl
}

void BcmSwitch::reconfigureCoPP(const StateDelta&) {
  // API not available in opennsl
}

void BcmSwitch::createAclGroup() {
  // API not available in opennsl
}

// Bcm's ContentAware Processing engine API is not open sourced yet
void BcmSwitch::processChangedAcl(
    const std::shared_ptr<AclEntry>& /*oldAcl*/,
    const std::shared_ptr<AclEntry>& /*newAcl*/) {}
void BcmSwitch::processAddedAcl(const std::shared_ptr<AclEntry>& /*acl*/) {}
void BcmSwitch::processRemovedAcl(const std::shared_ptr<AclEntry>& /*acl*/) {}

BcmSwitch::MmuState BcmSwitch::queryMmuState() const {
  return MmuState::UNKNOWN;
}

opennsl_gport_t BcmSwitch::getCpuGPort() const {
  // API not available in opennsl
  return 0;
}

void BcmSwitch::printDiagCmd(const std::string& /*cmd*/) const {}

void BcmSwitch::forceLinkscanOn(opennsl_pbmp_t /*ports*/) {}

static int _addL2Entry(int /*unit*/, opennsl_l2_addr_t* l2addr,
                       void* user_data) {
  L2EntryThrift entry;
  auto l2Table = static_cast<std::vector<L2EntryThrift>*>(user_data);
  entry.mac = folly::sformat("{0:02x}:{1:02x}:{2:02x}:{3:02x}:{4:02x}:{5:02x}",
                             l2addr->mac[0], l2addr->mac[1], l2addr->mac[2],
                             l2addr->mac[3], l2addr->mac[4], l2addr->mac[5]);
  entry.vlanID = l2addr->vid;
  entry.port = l2addr->port;
  XLOG(DBG6) << "L2 entry: Mac:" << entry.mac << " Vid:" << entry.vlanID
             << " Port:" << entry.port;
  l2Table->push_back(entry);
  return 0;
}

void BcmSwitch::fetchL2Table(std::vector<L2EntryThrift> *l2Table) {
  int rv = opennsl_l2_traverse(unit_, _addL2Entry, l2Table);
  bcmCheckError(rv, "opennsl_l2_traverse failed");
}

}} //facebook::fboss
