// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/TCPPacket.h"
#include "fboss/agent/packet/UDPDatagram.h"

#include <optional>
namespace facebook::fboss {

class HwSwitch;

namespace utility {
template <typename AddrT>
class IPPacket {
 public:
  using HdrT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      IPv4Hdr,
      IPv6Hdr>;
  // read entire ip packet, and populate payloads, useful to parse RxPacket
  explicit IPPacket(folly::io::Cursor& cursor);

  // set header fields, useful to construct TxPacket
  explicit IPPacket(const HdrT& hdr) : hdr_{hdr} {}

  IPPacket(const HdrT& hdr, const UDPDatagram& payload)
      : hdr_{hdr}, udpPayLoad_(payload) {
    fillIPHdr();
  }
  IPPacket(const HdrT& hdr, const TCPPacket& payload)
      : hdr_{hdr}, tcpPayLoad_(payload) {
    fillIPHdr();
  }
  size_t length() const {
    return hdr_.size() + payloadLength();
  }
  HdrT header() const {
    return hdr_;
  }

  const std::optional<UDPDatagram>& udpPayload() const {
    return udpPayLoad_;
  }
  const std::optional<TCPPacket>& tcpPayload() const {
    return tcpPayLoad_;
  }

  // construct TxPacket by encapsulating udp payload
  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
          allocatePacket) const;

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  bool operator==(const IPPacket<AddrT>& that) const {
    return std::tie(hdr_, udpPayLoad_, tcpPayLoad_, ipPayload_) ==
        std::tie(
               that.hdr_, that.udpPayLoad_, that.tcpPayLoad_, that.ipPayload_);
  }
  void decrementTTL() {
    hdr_.decrementTTL();
  }
  std::string toString() const;

 private:
  size_t payloadLength() const {
    if (udpPayLoad_) {
      return udpPayLoad_->length();
    } else if (tcpPayLoad_) {
      return tcpPayLoad_->length();
    } else if (ipPayload_) {
      return ipPayload_->size();
    }
    return 0;
  }

  size_t protocol() const {
    if constexpr (std::is_same_v<HdrT, IPv4Hdr>) {
      return hdr_.protocol;
    } else {
      return hdr_.nextHeader;
    }
  }

  void fillIPHdr() {
    if constexpr (std::is_same_v<HdrT, IPv4Hdr>) {
      hdr_.version = 4;
      hdr_.length = length();
      hdr_.ihl = (5 > hdr_.ihl) ? 5 : hdr_.ihl;
      if (!hdr_.ttl) {
        hdr_.ttl = 128;
      }
      hdr_.computeChecksum();
      hdr_.protocol = protocol();
    } else {
      hdr_.version = 6;
      hdr_.payloadLength = payloadLength();
      if (!hdr_.hopLimit) {
        hdr_.hopLimit = 128;
      }
      hdr_.nextHeader = protocol();
    }
  }

  void setUDPCheckSum(folly::IOBuf* buffer) const;
  HdrT hdr_;
  std::optional<UDPDatagram> udpPayLoad_;
  std::optional<TCPPacket> tcpPayLoad_;
  std::optional<std::vector<uint8_t>> ipPayload_;
};

template <typename AddrT>
inline std::ostream& operator<<(
    std::ostream& os,
    const IPPacket<AddrT>& ipPkt) {
  os << ipPkt.toString();
  return os;
}

using IPv4Packet = IPPacket<folly::IPAddressV4>;
using IPv6Packet = IPPacket<folly::IPAddressV6>;

} // namespace utility
} // namespace facebook::fboss
