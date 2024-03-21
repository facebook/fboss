// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/TCPPacket.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss::utility {
TCPPacket::TCPPacket(folly::io::Cursor& cursor, uint32_t length) {
  tcpHdr_.parse(&cursor);
  auto payLoadSize = length - TCPHeader::size(); // payload length
  payload_.resize(payLoadSize);
  for (auto i = 0; i < payLoadSize; i++) {
    payload_[i] = cursor.readBE<uint8_t>(); // payload bytes
  }
}

TCPPacket::~TCPPacket() = default;

std::unique_ptr<facebook::fboss::TxPacket> TCPPacket::getTxPacket(
    std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
        allocatePacket) const {
  auto txPacket = allocatePacket(length());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  tcpHdr_.write(&rwCursor);
  for (auto byte : payload_) {
    rwCursor.writeBE<uint8_t>(byte);
  }
  return txPacket;
}

void TCPPacket::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  tcpHdr_.write(&cursor);
  for (auto byte : payload_) {
    cursor.writeBE<uint8_t>(byte);
  }
}

bool TCPPacket::operator==(const TCPPacket& r) const {
  // ignore csum
  return std::tie(
             tcpHdr_.srcPort,
             tcpHdr_.dstPort,
             tcpHdr_.sequenceNumber,
             tcpHdr_.ackNumber,
             tcpHdr_.dataOffsetAndReserved,
             tcpHdr_.windowSize,
             tcpHdr_.urgentPointer,
             payload_) ==
      std::tie(
             r.tcpHdr_.srcPort,
             r.tcpHdr_.dstPort,
             r.tcpHdr_.sequenceNumber,
             r.tcpHdr_.ackNumber,
             r.tcpHdr_.dataOffsetAndReserved,
             r.tcpHdr_.windowSize,
             r.tcpHdr_.urgentPointer,
             r.payload_);
}
} // namespace facebook::fboss::utility
