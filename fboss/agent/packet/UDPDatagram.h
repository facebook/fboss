// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/io/Cursor.h>
#include <optional>
#include "fboss/agent/packet/UDPHeader.h"

namespace facebook::fboss {
class TxPacket;
class HwSwitch;

namespace utility {
class UDPDatagram {
 public:
  // read entire udp datagram, and populate payloads, useful to parse RxPacket
  explicit UDPDatagram(folly::io::Cursor& cursor);

  // set header fields, useful to construct TxPacket
  UDPDatagram(const UDPHeader& udpHdr, const std::vector<uint8_t>& payload)
      : udpHdr_(udpHdr), payload_(payload) {
    udpHdr_.length = udpHdr_.size() + payload_.size();
  }

  ~UDPDatagram();
  size_t length() const {
    return UDPHeader::size() + payload_.size();
  }

  const UDPHeader& header() const {
    return udpHdr_;
  }

  const std::vector<uint8_t>& payload() const {
    return payload_;
  }

  // construct TxPacket by encapsulating rabdom byte payload
  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
          allocatePacket) const;

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  bool operator==(const UDPDatagram& that) const {
    /* ignore checksum, */
    return std::tie(
               udpHdr_.srcPort, udpHdr_.dstPort, udpHdr_.length, payload_) ==
        std::tie(
               that.udpHdr_.srcPort,
               that.udpHdr_.dstPort,
               that.udpHdr_.length,
               that.payload_);
  }
  bool operator!=(const UDPDatagram& that) const {
    return !(*this == that);
  }
  std::string toString() const {
    return udpHdr_.toString();
  }

 private:
  UDPHeader udpHdr_;
  std::vector<uint8_t> payload_{};
};

inline std::ostream& operator<<(std::ostream& os, const UDPDatagram& udp) {
  os << udp.toString();
  return os;
}

} // namespace utility
} // namespace facebook::fboss
