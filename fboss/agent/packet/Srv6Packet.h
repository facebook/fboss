// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include "fboss/agent/packet/IPPacket.h"

namespace facebook::fboss::utility {

class Srv6Packet {
 public:
  // Parse from cursor: reads outer IPv6 header, then inner packet based on
  // nextHeader
  explicit Srv6Packet(folly::io::Cursor& cursor);

  // Construct from outer header only
  explicit Srv6Packet(IPv6Hdr hdr) : hdr_(std::move(hdr)) {}

  // Construct with inner payload
  Srv6Packet(IPv6Hdr hdr, IPPacket<folly::IPAddressV4> payload)
      : hdr_(std::move(hdr)), v4PayLoad_(std::move(payload)) {}
  Srv6Packet(IPv6Hdr hdr, IPPacket<folly::IPAddressV6> payload)
      : hdr_(std::move(hdr)), v6PayLoad_(std::move(payload)) {}

  IPv6Hdr header() const {
    return hdr_;
  }

  size_t length() const {
    return IPv6Hdr::SIZE +
        (v4PayLoad_ ? v4PayLoad_->length()
                    : (v6PayLoad_ ? v6PayLoad_->length() : 0));
  }

  const std::optional<IPPacket<folly::IPAddressV4>>& v4PayLoad() const {
    return v4PayLoad_;
  }

  const std::optional<IPPacket<folly::IPAddressV6>>& v6PayLoad() const {
    return v6PayLoad_;
  }

  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
          allocatePacket) const;

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  bool operator==(const Srv6Packet& that) const {
    return std::tie(hdr_, v4PayLoad_, v6PayLoad_) ==
        std::tie(that.hdr_, that.v4PayLoad_, that.v6PayLoad_);
  }

  std::string toString() const;

 private:
  IPv6Hdr hdr_;
  std::optional<IPPacket<folly::IPAddressV4>> v4PayLoad_;
  std::optional<IPPacket<folly::IPAddressV6>> v6PayLoad_;
};

inline std::ostream& operator<<(std::ostream& os, const Srv6Packet& srv6Pkt) {
  os << srv6Pkt.toString();
  return os;
}

} // namespace facebook::fboss::utility
