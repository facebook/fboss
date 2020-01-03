/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <array>
#include <memory>
#include <unordered_set>
#include <vector>
#include "fboss/agent/types.h"

using folly::IPAddressV6;
using folly::MacAddress;

namespace folly {
namespace io {
class Cursor;
}
} // namespace folly

namespace facebook::fboss {

enum class DHCPv6Type : uint8_t {
  DHCPv6_SOLICIT = 1,
  DHCPv6_ADVERTISE = 2,
  DHCPv6_REQUEST = 3,
  DHCPv6_CONFIRM = 4,
  DHCPv6_RENEW = 5,
  DHCPv6_REBIND = 6,
  DHCPv6_REPLY = 7,
  DHCPv6_RELEASE = 8,
  DHCPv6_DECLINE = 9,
  DHCPv6_RECONFIGURE = 10,
  DHCPv6_INFORM_REQ = 11,
  DHCPv6_RELAY_FORWARD = 12,
  DHCPv6_RELAY_REPLY = 13,
};

enum class DHCPv6OptionType : uint16_t {
  DHCPv6_OPTION_RELAY_MSG = 9,
  DHCPv6_OPTION_INTERFACE_ID = 18,
};

struct DHCPv6Option;

struct DHCPv6Packet {
  typedef std::vector<uint8_t> Options;
  enum { DHCP6_CLIENT_UDPPORT = 546 };
  enum { DHCP6_SERVERAGENT_UDPPORT = 547 };
  // IPV6_MIN_MTU - EthHdr - IPv6Hdr - UDPHdr
  enum { MAX_DHCPV6_MSG_LENGTH = 1214 };

  enum { TYPE_BYTES = 1 };
  enum { HOPCOUNT_BYTES = 1 };
  enum { LINKADDR_BYTES = 16 };
  enum { PEERADDR_BYTES = 16 };
  enum { TRANSACTIONID_BYTES = 3 };

 public:
  DHCPv6Packet() {}

  DHCPv6Packet(
      uint8_t _type,
      uint8_t _hopCount,
      IPAddressV6 _la,
      IPAddressV6 _pa)
      : type(_type), hopCount(_hopCount), linkAddr(_la), peerAddr(_pa) {}

  DHCPv6Packet(uint8_t _type, uint32_t _tid)
      : type(_type), transactionId(_tid) {}

  void parse(folly::io::Cursor* cursor);

  template <typename CursorType>
  void write(CursorType* cursor) const;

  void addRelayMessageOption(const DHCPv6Packet& dhcpPktIn);
  void addInterfaceIDOption(MacAddress macAddr);

  std::vector<DHCPv6Option> extractOptions(
      std::unordered_set<uint16_t> optionSelector);
  size_t appendOption(uint16_t op, uint16_t len, const uint8_t* bytes);
  size_t computePacketLength() const;
  bool isDHCPv6Relay() const;
  bool operator==(const DHCPv6Packet& r) const;
  std::string toString() const;

 public:
  /* From rfc 3315
   * DHCPv6 packet:
   * 0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  type (1)     |             transactionid (3)                 |
    +---------------+---------------+---------------+---------------+
    |                          options (variable)                   |
    +---------------------------------------------------------------+
   *
   * DHCPv6 relay packet:
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  type (1)     |   hopcnt (1)  |                               |
    +---------------+---------------+---------------+---------------+
    |                       Link-address (16)                       |
    +-------------------------------+-------------------------------+
    |                                                               |
    +-------------------------------+-------------------------------+
    |                               |                               |
    +---------------------------------------------------------------+
    |                      Peer-address (16)                        |
    +---------------------------------------------------------------+
    |                                                               |
    +---------------------------------------------------------------+
    |                               |                               |
    +---------------------------------------------------------------+
    |                          options (variable)                   |
    +---------------------------------------------------------------+
   */
  uint8_t type;
  uint8_t hopCount;
  // this is an opaque value in host byte order, but when we read and write
  // it to the wire, the data is in network order.
  uint32_t transactionId;
  folly::IPAddressV6 linkAddr;
  folly::IPAddressV6 peerAddr;
  Options options;
};

template <typename CursorType>
void DHCPv6Packet::write(CursorType* cursor) const {
  if (isDHCPv6Relay()) {
    // this is a DHCPv6 relay packet
    cursor->template write<uint8_t>(type);
    cursor->template write<uint8_t>(hopCount);
    cursor->push(linkAddr.bytes(), 16);
    cursor->push(peerAddr.bytes(), 16);
  } else {
    uint32_t firstWord = (type << 24) | (transactionId & 0x00ffffff);
    cursor->template writeBE<uint32_t>(firstWord);
  }
  if (options.size() > 0) {
    cursor->push(&options[0], options.size());
  }
}

struct DHCPv6Option {
 public:
  DHCPv6Option() {}
  DHCPv6Option(uint16_t _op, uint16_t _len, const uint8_t* _data)
      : op(_op), len(_len), data(_data) {}

  void parse(DHCPv6Packet::Options& optionsIn, int index);

 public:
  uint16_t op;
  uint16_t len;
  const uint8_t* data;
};

} // namespace facebook::fboss
