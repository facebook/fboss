// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/MPLSPacket.h"

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss::utility {

MPLSPacket::MPLSPacket(folly::io::Cursor& cursor) {
  hdr_ = MPLSHdr(&cursor);
  uint8_t ipver = 0;
  if (cursor.tryReadBE<uint8_t>(ipver)) {
    cursor.retreat(1);
    ipver >>= 4; // ip version is in first four bits
    if (ipver == 4) {
      v4PayLoad_ = IPPacket<folly::IPAddressV4>(cursor);
    } else if (ipver == 6) {
      v6PayLoad_ = IPPacket<folly::IPAddressV6>(cursor);
    }
  }
}

std::unique_ptr<facebook::fboss::TxPacket> MPLSPacket::getTxPacket(
    std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
        allocatePacket) const {
  auto txPacket = allocatePacket(length());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());

  hdr_.serialize(&rwCursor);
  if (v4PayLoad_) {
    auto v4Packet = v4PayLoad_->getTxPacket(allocatePacket);
    folly::io::Cursor cursor(v4Packet->buf());
    rwCursor.push(cursor, v4PayLoad_->length());
  } else if (v6PayLoad_) {
    auto v6Packet = v6PayLoad_->getTxPacket(allocatePacket);
    folly::io::Cursor cursor(v6Packet->buf());
    rwCursor.push(cursor, v6PayLoad_->length());
  }
  return txPacket;
}

void MPLSPacket::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  hdr_.serialize(&cursor);
  if (v4PayLoad_) {
    v4PayLoad_->serialize(cursor);
  } else if (v6PayLoad_) {
    v6PayLoad_->serialize(cursor);
  }
}

std::string MPLSPacket::toString() const {
  std::stringstream ss;
  ss << "MPLS hdr: " << hdr_.toString()
     << " v6 : " << (v6PayLoad_.has_value() ? v6PayLoad_->toString() : "")
     << " v4 : " << (v4PayLoad_.has_value() ? v4PayLoad_->toString() : "");
  return ss.str();
}

} // namespace facebook::fboss::utility
