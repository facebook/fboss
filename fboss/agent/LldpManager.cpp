/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2014-present Facebook. All Rights Reserved.
#include "fboss/agent/LldpManager.h"

#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/futures/Future.h>
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include <unistd.h>
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"

using folly::ByteRange;
using folly::MacAddress;
using folly::StringPiece;
using folly::io::RWPrivateCursor;
using std::shared_ptr;

/**
 * False if the given LLDP tag has an Expected value configured, and
 * the value received was not as expected.
 *
 * True otherwise.
 */
bool checkTag(
    facebook::fboss::PortID id,
    const facebook::fboss::Port::LLDPValidations& lldpmap,
    facebook::fboss::cfg::LLDPTag tag,
    const std::string& val) {
  auto res = lldpmap.find(tag);

  if (res == lldpmap.end()) {
    XLOG(DBG4) << "Port " << id << ": " << std::to_string(static_cast<int>(tag))
               << ": Not present in config";
    return true;
  }

  auto expect = res->second;
  if (val.find(expect) != std::string::npos) {
    XLOG(DBG4) << "Port " << id << std::to_string(static_cast<int>(tag))
               << ", matches: \"" << expect << "\", got: \"" << val << "\"";
    return true;
  }

  XLOG(WARNING) << "Port " << id << ", LLDP tag "
                << std::to_string(static_cast<int>(tag)) << ", expected: \""
                << expect << "\", got: \"" << val << "\"";
  return false;
}

namespace facebook::fboss {

const MacAddress LldpManager::LLDP_DEST_MAC("01:80:c2:00:00:0e");

LldpManager::LldpManager(SwSwitch* sw)
    : folly::AsyncTimeout(sw->getBackgroundEvb()),
      sw_(sw),
      intervalMsecs_(LLDP_INTERVAL) {}

LldpManager::~LldpManager() {}

void LldpManager::start() {
  sw_->getBackgroundEvb()->runInEventBaseThread(
      [this] { this->timeoutExpired(); });
}

void LldpManager::stop() {
  sw_->getBackgroundEvb()->runInEventBaseThreadAndWait(
      [this] { this->cancelTimeout(); });
}

void LldpManager::handlePacket(
    std::unique_ptr<RxPacket> pkt,
    folly::MacAddress /*dst*/,
    folly::MacAddress src,
    folly::io::Cursor cursor) {
  auto neighbor = std::make_shared<LinkNeighbor>();

  sw_->stats()->LldpRecvdPkt();

  bool ret = neighbor->parseLldpPdu(
      pkt->getSrcPort(), pkt->getSrcVlanIf(), src, ETHERTYPE_LLDP, &cursor);

  if (!ret) {
    // LinkNeighbor will have already logged a message about the error.
    // Just ignore the packet.
    sw_->stats()->LldpBadPkt();
    return;
  }

  XLOG(DBG4) << "got LLDP packet: local_port=" << pkt->getSrcPort()
             << " chassis=" << neighbor->humanReadableChassisId()
             << " port=" << neighbor->humanReadablePortId()
             << " name=" << neighbor->getSystemName();

  auto plport = sw_->getPlatform()->getPlatformPort(pkt->getSrcPort());
  auto port = sw_->getState()->getPorts()->getPortIf(pkt->getSrcPort());
  PortID pid = pkt->getSrcPort();

  XLOG(DBG4) << "Port " << pid << ", local name: " << port->getName()
             << ", local desc: " << port->getDescription()
             << ", LLDP systemname: " << neighbor->getSystemName()
             << ", LLDP hrPort: " << neighbor->humanReadablePortId()
             << ", LLDP dsc: " << neighbor->getPortDescription();

  auto lldpmap = port->getLLDPValidations();
  if (!(checkTag(
            pid,
            lldpmap,
            cfg::LLDPTag::SYSTEM_NAME,
            neighbor->getSystemName()) &&
        checkTag(
            pid,
            lldpmap,
            cfg::LLDPTag::PORT,
            neighbor->humanReadablePortId()))) {
    sw_->stats()->LldpValidateMisMatch();
    // TODO(pjakma): Figure out how to mock plport
    if (plport) {
      plport->externalState(PortLedExternalState::CABLING_ERROR);
    }
    XLOG(DBG4) << "LLDP expected/recvd value mismatch!";
  }
  db_.update(neighbor);
}

void LldpManager::timeoutExpired() noexcept {
  try {
    sendLldpOnAllPorts();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to send LLDP on all ports. Error:"
              << folly::exceptionStr(ex);
  }
  scheduleTimeout(intervalMsecs_);
}

void LldpManager::sendLldpOnAllPorts() {
  // send lldp frames through all the ports here.
  std::shared_ptr<SwitchState> state = sw_->getState();
  for (const auto& port : std::as_const(*state->getPorts())) {
    if (port.second->getPortType() == cfg::PortType::INTERFACE_PORT &&
        port.second->isPortUp()) {
      sendLldpInfo(port.second);
    } else {
      XLOG(DBG5) << "Skipping LLDP send on port: " << port.second->getID();
    }
  }
}

uint16_t tlvHeader(uint16_t type, uint16_t length) {
  DCHECK_EQ((type & ~0x7f), 0);
  DCHECK_EQ((length & ~0x01ff), 0);
  return (type << 9) | length;
}

void writeTl(LldpTlvType type, uint16_t length, RWPrivateCursor* cursor) {
  uint16_t typeLength = tlvHeader(static_cast<uint16_t>(type), length);
  cursor->writeBE<uint16_t>(typeLength);
}

void writeTlv(
    LldpTlvType type,
    uint8_t subType,
    const ByteRange& buf,
    RWPrivateCursor* cursor) {
  uint16_t length = buf.size() + 1;
  writeTl(type, length, cursor);
  cursor->writeBE<uint8_t>(subType);
  cursor->push(buf.data(), (size_t)buf.size());
}

void writeTlv(
    LldpTlvType type,
    lldp::LldpChassisIdType subType,
    const ByteRange& buf,
    RWPrivateCursor* cursor) {
  writeTlv(type, static_cast<uint8_t>(subType), buf, cursor);
}

void writeTlv(
    LldpTlvType type,
    lldp::LldpPortIdType subType,
    const ByteRange& buf,
    RWPrivateCursor* cursor) {
  writeTlv(type, static_cast<uint8_t>(subType), buf, cursor);
}

void writeTlv(LldpTlvType type, uint16_t value, RWPrivateCursor* cursor) {
  writeTl(type, 2, cursor);
  cursor->writeBE<uint16_t>(value);
}

void writeTlv(LldpTlvType type, uint32_t value, RWPrivateCursor* cursor) {
  writeTl(type, 4, cursor);
  cursor->writeBE<uint32_t>(value);
}

void writeTlv(
    LldpTlvType type,
    const ByteRange& value,
    RWPrivateCursor* cursor) {
  writeTl(type, value.size(), cursor);
  cursor->push(value.data(), value.size());
}

uint32_t LldpManager::LldpPktSize(
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const std::string& sysDesc) {
  return
      // ethernet header
      (6 * 2) + 2 +
      2
      // LLDP TLVs
      // Chassis MAC
      + 2 + 1 +
      6
      // Port ID: Name
      + 2 + portname.size() +
      1
      // TTL
      + 2 +
      2
      // Add Port-Description length
      + 2 + portdesc.size() +
      1
      // system name
      + 2 + hostname.size() +
      1
      // System description
      + 2 + sysDesc.size() +
      1
      // Capabilities
      + 2 + 2 +
      2
      // End of LLDPDU
      + 2;
}

void LldpManager::fillLldpTlv(
    TxPacket* pkt,
    const MacAddress macaddr,
    VlanID vlanid,
    const std::string& systemdescr,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities) {
  RWPrivateCursor cursor(pkt->buf());
  pkt->writeEthHeader(&cursor, LLDP_DEST_MAC, macaddr, vlanid, ETHERTYPE_LLDP);
  // now write chassis ID TLV
  writeTlv(
      LldpTlvType::CHASSIS,
      lldp::LldpChassisIdType::MAC_ADDRESS,
      ByteRange(macaddr.bytes(), 6),
      &cursor);

  // now write port ID TLV
  /* using StringPiece here to bridge chars in string to unsigned chars in
   * ByteRange.
   */
  writeTlv(
      LldpTlvType::PORT,
      lldp::LldpPortIdType::INTERFACE_NAME,
      StringPiece(portname),
      &cursor);

  // now write TTL TLV
  writeTlv(LldpTlvType::TTL, ttl, &cursor);

  // now write optional TLVs
  // system name TLV
  if (hostname.size() > 0) {
    writeTlv(LldpTlvType::SYSTEM_NAME, StringPiece(hostname), &cursor);
  }

  // Port description
  writeTlv(LldpTlvType::PORT_DESC, StringPiece(portdesc), &cursor);

  // system description TLV
  writeTlv(LldpTlvType::SYSTEM_DESCRIPTION, StringPiece(systemdescr), &cursor);

  // system capability TLV
  uint32_t enabledCapabilities = (capabilities << 16) | capabilities;
  writeTlv(LldpTlvType::SYSTEM_CAPABILITY, enabledCapabilities, &cursor);

  // now write PDU End TLV
  writeTl(LldpTlvType::PDU_END, PDU_END_TLV_LENGTH, &cursor);

  // Fill the padding with 0s
  memset(cursor.writableData(), 0, cursor.length());
}

std::unique_ptr<TxPacket> LldpManager::createLldpPkt(
    SwSwitch* sw,
    const MacAddress macaddr,
    VlanID vlanid,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities) {
  static std::string lldpSysDescStr("FBOSS");
  uint32_t frameLen = LldpPktSize(hostname, portname, portdesc, lldpSysDescStr);

  auto pkt = sw->allocatePacket(frameLen);
  fillLldpTlv(
      pkt.get(),
      macaddr,
      vlanid,
      lldpSysDescStr,
      hostname,
      portname,
      portdesc,
      ttl,
      capabilities);
  return pkt;
}

void LldpManager::sendLldpInfo(const std::shared_ptr<Port>& port) {
  MacAddress cpuMac = sw_->getPlatform()->getLocalMac();
  PortID thisPortID = port->getID();

  const size_t kMaxLen = 64;
  std::array<char, kMaxLen> hostname;
  if (0 == gethostname(hostname.data(), kMaxLen)) {
    // make sure it is null terminated
    hostname[kMaxLen - 1] = '\0';
  } else {
    hostname[0] = '\0';
  }

  auto pkt = LldpManager::createLldpPkt(
      sw_,
      cpuMac,
      port->getIngressVlan(),
      std::string(hostname.data()),
      port->getName(),
      port->getDescription(),
      TTL_TLV_VALUE,
      SYSTEM_CAPABILITY_ROUTER);

  // this LLDP packet HAS to exit out of the port specified here.
  sw_->sendNetworkControlPacketAsync(
      std::move(pkt), PortDescriptor(thisPortID));

  XLOG(DBG4) << "sent LLDP "
             << " on port " << port->getID() << " with CPU MAC "
             << cpuMac.toString() << " port id " << port->getName()
             << " and vlan " << port->getIngressVlan();
}

} // namespace facebook::fboss
