// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/Srv6Packet.h"

#include <sstream>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/IPProto.h"

namespace facebook::fboss::utility {

Srv6Packet::Srv6Packet(folly::io::Cursor& cursor) {
  hdr_ = IPv6Hdr(cursor);
  if (hdr_.nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV4)) {
    v4PayLoad_ = IPPacket<folly::IPAddressV4>(cursor);
  } else if (hdr_.nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6)) {
    v6PayLoad_ = IPPacket<folly::IPAddressV6>(cursor);
  } else {
    throw FbossError(
        "SRv6 packet with unsupported nextHeader: ",
        static_cast<int>(hdr_.nextHeader));
  }
}

std::unique_ptr<facebook::fboss::TxPacket> Srv6Packet::getTxPacket(
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

void Srv6Packet::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  hdr_.serialize(&cursor);
  if (v4PayLoad_) {
    v4PayLoad_->serialize(cursor);
  } else if (v6PayLoad_) {
    v6PayLoad_->serialize(cursor);
  }
}

std::string Srv6Packet::toString() const {
  std::stringstream ss;
  ss << "SRv6 hdr: " << hdr_.toString()
     << " v6 : " << (v6PayLoad_.has_value() ? v6PayLoad_->toString() : "")
     << " v4 : " << (v4PayLoad_.has_value() ? v4PayLoad_->toString() : "");
  return ss.str();
}

} // namespace facebook::fboss::utility
