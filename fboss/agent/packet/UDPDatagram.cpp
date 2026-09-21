// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/UDPDatagram.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss::utility {
UDPDatagram::UDPDatagram(folly::io::Cursor& cursor) {
  udpHdr_.parse(&cursor, nullptr);
  auto payLoadSize = udpHdr_.length - UDPHeader::size(); // payload length
  payload_.resize(payLoadSize);
  for (auto i = 0; i < payLoadSize; i++) {
    payload_[i] = cursor.readBE<uint8_t>(); // payload bytes
  }
}

UDPDatagram::~UDPDatagram() = default;

std::unique_ptr<facebook::fboss::TxPacket> UDPDatagram::getTxPacket(
    std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
        allocatePacket) const {
  auto txPacket = allocatePacket(length());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  udpHdr_.write(&rwCursor);
  for (auto byte : payload_) {
    rwCursor.writeBE<uint8_t>(byte);
  }
  return txPacket;
}

void UDPDatagram::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  udpHdr_.write(&cursor);
  for (auto byte : payload_) {
    cursor.writeBE<uint8_t>(byte);
  }
}
} // namespace facebook::fboss::utility
