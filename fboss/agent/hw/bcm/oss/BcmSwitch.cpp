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

#include "fboss/agent/hw/BufferStatsLogger.h"
#include "fboss/agent/hw/bcm/BcmRxPacket.h"

#include <folly/Memory.h>

extern "C" {
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

void BcmSwitch::configureAdditionalEcmpHashSets() {}

bool BcmSwitch::isAlpmEnabled() {
  return false;
}

void BcmSwitch::dropDhcpPackets() {}

void BcmSwitch::setupCos() {}

void BcmSwitch::setupChangedOrMissingFPGroups() {
}

void BcmSwitch::copyIPv6LinkLocalMcastPackets() {
  // OpenNSL doesn't yet provide functions for adding field-processor rules
  // for capturing packets
}

void BcmSwitch::configureRxRateLimiting() {
  // OpenNSL doesn't yet provide functions for configuring rate-limiting,
  // so rate limiting settings must be baked into the binary driver.
}

void BcmSwitch::createSlowProtocolsGroup() {
  // OpenNSL doesn't yet provide functions for adding field-processor rules
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

void BcmSwitch::dumpState() const {}

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

int BcmSwitch::getHighresSamplers(
    HighresSamplerList* /*samplers*/,
    const std::string& /*namespaceString*/,
    const std::set<CounterRequest>& /*counterSet*/) {
  return 0;
}

void BcmSwitch::exportSdkVersion() const {}

void BcmSwitch::fetchL2Table(std::vector<L2EntryThrift>* /*l2Table*/) {
  return;
}

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

bool BcmSwitch::startBufferStatCollection() {
  LOG(INFO) << "Buffer stats collection not supported";
  return bufferStatsEnabled_;
}
bool BcmSwitch::stopBufferStatCollection() {
  LOG(INFO) << "no op, buffer stats collection is not supported";
  return !bufferStatsEnabled_;
}
void BcmSwitch::exportDeviceBufferUsage() {}

std::unique_ptr<BufferStatsLogger> BcmSwitch::createBufferStatsLogger() {
  return std::make_unique<GlogBufferStatsLogger>();
}

void BcmSwitch::printDiagCmd(const std::string& cmd) const {}

void BcmSwitch::forceLinkscanOn(opennsl_pbmp_t ports) {}

}} //facebook::fboss
