// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/io/Cursor.h>
#include <optional>
#include "fboss/agent/packet/TCPHeader.h"

namespace facebook::fboss {
class TxPacket;
class HwSwitch;

namespace utility {
class TCPPacket {
 public:
  // read entire tcp datagram, and populate payloads, useful to parse RxPacket
  explicit TCPPacket(folly::io::Cursor& cursor, uint32_t length);

  // set header fields, useful to construct TxPacket
  TCPPacket(const TCPHeader& tcpHdr, const std::vector<uint8_t>& payload)
      : tcpHdr_(tcpHdr), payload_(payload) {}

  ~TCPPacket();
  size_t length() const {
    return TCPHeader::size() + payload_.size();
  }

  const TCPHeader& header() const {
    return tcpHdr_;
  }

  const std::vector<uint8_t>& payload() const {
    return payload_;
  }

  // construct TxPacket by encapsulating random byte payload
  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
          allocatePacket) const;

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  /* ignores checksum, */
  bool operator==(const TCPPacket& that) const;
  bool operator!=(const TCPPacket& that) const {
    return !(*this == that);
  }
  std::string toString() const {
    return tcpHdr_.toString();
  }

 private:
  TCPHeader tcpHdr_;
  std::vector<uint8_t> payload_{};
};

inline std::ostream& operator<<(std::ostream& os, const TCPPacket& tcp) {
  os << tcp.toString();
  return os;
}

} // namespace utility
} // namespace facebook::fboss
