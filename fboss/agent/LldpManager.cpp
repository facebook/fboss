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

#include <folly/futures/Future.h>
#include <folly/io/Cursor.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include <unistd.h>

using folly::MacAddress;
using folly::io::RWPrivateCursor;
using folly::ByteRange;
using folly::StringPiece;
using std::shared_ptr;


namespace facebook { namespace fboss {

const MacAddress LldpManager::LLDP_DEST_MAC("01:80:c2:00:00:0e");

LldpManager::LldpManager(SwSwitch* sw)
  : folly::AsyncTimeout(sw->getBackgroundEVB()),
    sw_(sw),
    interval_(LLDP_INTERVAL) {}

LldpManager::~LldpManager() {}

void LldpManager::start() {
  sw_->getBackgroundEVB()->runInEventBaseThread([this] {
    this->timeoutExpired();
  });
}

void LldpManager::stop() {
  auto f = via(sw_->getBackgroundEVB())
    .then([this] { this->cancelTimeout(); });
  f.get();
}

void LldpManager::handlePacket(std::unique_ptr<RxPacket> pkt,
                               folly::MacAddress dst,
                               folly::MacAddress src,
                               folly::io::Cursor cursor) {
  LinkNeighbor neighbor;
  bool ret = neighbor.parseLldpPdu(pkt->getSrcPort(), pkt->getSrcVlan(),
                                   src, ETHERTYPE_LLDP, &cursor);
  if (!ret) {
    // LinkNeighbor will have already logged a message about the error.
    // Just ignore the packet.
    return;
  }

  VLOG(4) << "got LLDP packet: local_port=" << pkt->getSrcPort() <<
    " chassis=" << neighbor.humanReadableChassisId() <<
    " port=" << neighbor.humanReadablePortId() <<
    " name=" << neighbor.getSystemName();
  db_.update(neighbor);
}

void LldpManager::timeoutExpired() noexcept {
  try {
    sendLldpOnAllPorts(true);
  } catch (const std::exception& ex) {
    LOG(ERROR) << "Failed to send LLDP on all ports. Error:"
               << folly::exceptionStr(ex);
  }
  scheduleTimeout(interval_);
}

void LldpManager::sendLldpOnAllPorts(bool checkPortStatusFlag) {
  // send lldp frames through all the ports here.
  std::shared_ptr<SwitchState> state = sw_->getState();
  for (const auto& port : *state->getPorts()) {
    if (checkPortStatusFlag == false || sw_->isPortUp(port->getID())) {
      sendLldpInfo(sw_, state, port);
    } else {
      VLOG(5) << "Skipping LLDP send as this port is disabled " <<
        port->getID();
    }
  }
}

uint16_t tlvHeader(uint16_t type, uint16_t length) {
  DCHECK_EQ((type & ~0x7f), 0);
  DCHECK_EQ((length & ~0x01ff), 0);
  return (type << 9) | length;
}

void writeTl(uint16_t type, uint16_t length, RWPrivateCursor* cursor) {
  uint16_t typeLength = tlvHeader(type, length);
  cursor->writeBE<uint16_t>(typeLength);
}

void writeTlv(uint16_t type, uint8_t subType, const ByteRange& buf,
              RWPrivateCursor* cursor) {
  uint16_t length = buf.size() + 1;
  writeTl(type, length, cursor);
  cursor->writeBE<uint8_t>(subType);
  cursor->push(buf.data(), (size_t)buf.size());
}

void writeTlv(uint16_t type, uint16_t value,
              RWPrivateCursor* cursor) {
  writeTl(type, 2, cursor);
  cursor->writeBE<uint16_t>(value);
}

void writeTlv(uint16_t type, uint32_t value, RWPrivateCursor* cursor) {
  writeTl(type, 4, cursor);
  cursor->writeBE<uint32_t>(value);
}

void writeTlv(uint16_t type, const ByteRange& value,
              RWPrivateCursor* cursor) {
  writeTl(type, value.size(), cursor);
  cursor->push(value.data(), value.size());
}

void LldpManager::sendLldpInfo(SwSwitch* sw,
                               const std::shared_ptr<SwitchState>& swState,
                               const std::shared_ptr<Port>& port) {
  MacAddress cpuMac = sw->getPlatform()->getLocalMac();
  const size_t kMaxLen = 64;
  char hostname[kMaxLen];

  if (0 == gethostname(hostname, kMaxLen)) {
    // make sure it is null terminated
    hostname[kMaxLen - 1] = '\0';
  } else {
    hostname[0] = '\0';
  }

  // The minimum packet length is 64.We use 68 on the assumption that
  // the packet will go out untagged, which will remove 4 bytes.
  uint32_t frameLen = 98;
  auto pkt = sw->allocatePacket(frameLen);
  PortID thisPortID = port->getID();
  RWPrivateCursor cursor(pkt->buf());
  pkt->writeEthHeader(&cursor, LLDP_DEST_MAC,
                      cpuMac, port->getIngressVlan(), ETHERTYPE_LLDP);
  // now write chassis ID TLV
  writeTlv(CHASSIS_TLV_TYPE, CHASSIS_TLV_SUB_TYPE_MAC,
           ByteRange(cpuMac.bytes(), 6), &cursor);

  // now write port ID TLV
  /* using StringPiece here to bridge chars in string to unsigned chars in
   * ByteRange.
   */
  writeTlv(PORT_TLV_TYPE, PORT_TLV_SUB_TYPE_INTERFACE,
           StringPiece(port->getName()), &cursor);

  // now write TTL TLV
  writeTlv(TTL_TLV_TYPE, (uint16_t) TTL_TLV_VALUE, &cursor);

  // now write optional TLVs
  // system name TLV
  if (strlen(hostname) > 0) {
    writeTlv(SYSTEM_NAME_TLV_TYPE,
             StringPiece(hostname), &cursor);
  }

  // system description TLV
  writeTlv(SYSTEM_DESCRIPTION_TLV_TYPE, StringPiece("FBOSS"), &cursor);

  // system capability TLV
  uint16_t capability = SYSTEM_CAPABILITY_ROUTER;
  uint32_t systemAndEnabledCapability = (capability << 16) | capability;
  writeTlv(SYSTEM_CAPABILITY_TLV_TYPE, systemAndEnabledCapability, &cursor);

  // now write PDU End TLV
  writeTl(PDU_END_TLV_TYPE, PDU_END_TLV_LENGTH, &cursor);

  // Fill the padding with 0s
  memset(cursor.writableData(), 0, cursor.length());
  // this LLDP packet HAS to exit out of the port specified here.
  sw->sendPacketOutOfPort(std::move(pkt), thisPortID);
  VLOG(4) << "sent LLDP "
    << " on port " << port->getID()
    << " with CPU MAC " << cpuMac.toString()
    << " port id " << port->getName()
    << " and vlan " << port->getIngressVlan();
}

}} // facebook::fboss
