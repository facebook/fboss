/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "DHCPv6Packet.h"
#include <folly/IPAddress.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <sstream>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/packet/PktUtil.h"

using folly::IOBuf;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::back_inserter;
using std::copy;
using std::string;
using std::stringstream;
using std::vector;

namespace facebook::fboss {

class SwSwitch;

void DHCPv6Option::parse(DHCPv6Packet::Options& optionsIn, int index) {
  auto ioBuf = IOBuf::wrapBuffer(&optionsIn[index], optionsIn.size() - index);
  Cursor cursor(ioBuf.get());
  op = cursor.readBE<uint16_t>();
  len = cursor.readBE<uint16_t>();
  data = cursor.data();
}

bool DHCPv6Packet::isDHCPv6Relay() const {
  return (
      static_cast<DHCPv6Type>(type) == DHCPv6Type::DHCPv6_RELAY_FORWARD ||
      static_cast<DHCPv6Type>(type) == DHCPv6Type::DHCPv6_RELAY_REPLY);
}

void DHCPv6Packet::addRelayMessageOption(const DHCPv6Packet& dhcpPktIn) {
  // serialize the dhcpPktIn
  auto totalLength = dhcpPktIn.computePacketLength();
  uint8_t pktBuf[totalLength];
  auto buf = IOBuf::wrapBuffer(pktBuf, totalLength);
  RWPrivateCursor cursor(buf.get());
  dhcpPktIn.write(&cursor);

  // append the option
  appendOption(
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_RELAY_MSG),
      totalLength,
      buf->data());
}

void DHCPv6Packet::addInterfaceIDOption(MacAddress macAddr) {
  appendOption(
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID),
      MacAddress::SIZE,
      macAddr.bytes());
}

/**
 * DHCPv6 option format:
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      option-code (2)          |        option-len (2)         |
   +---------------+---------------+---------------+---------------+
   |                       option-data (variable)                  |
   +-------------------------------+-------------------------------+
*/
size_t
DHCPv6Packet::appendOption(uint16_t op, uint16_t len, const uint8_t* bytes) {
  IOBuf buf(IOBuf::CREATE, 4);
  buf.append(4);
  RWPrivateCursor cursor(&buf);
  // values in network byte order
  cursor.writeBE<uint16_t>(op);
  cursor.writeBE<uint16_t>(len);

  auto origSize = options.size();
  options.reserve(origSize + 4 + len);
  copy(buf.data(), buf.data() + 4, std::back_inserter(options));
  copy(bytes, bytes + len, std::back_inserter(options));

  CHECK(options.size() == origSize + 4 + len);
  return 4 + len;
}

void DHCPv6Packet::parse(Cursor* cursor) {
  try {
    type = cursor->read<uint8_t>();
    if (isDHCPv6Relay()) {
      // DHCPv6 relay packet
      hopCount = cursor->read<uint8_t>();
      linkAddr = PktUtil::readIPv6(cursor);
      peerAddr = PktUtil::readIPv6(cursor);
    } else {
      uint8_t high = cursor->read<uint8_t>();
      uint16_t low = cursor->readBE<uint16_t>();
      transactionId = (high << 16) + low;
    }
    if (cursor->totalLength()) {
      auto optionsLen = cursor->totalLength();
      uint8_t optionsRaw[optionsLen];
      options.reserve(options.size() + optionsLen);
      cursor->pullAtMost(optionsRaw, optionsLen);
      copy(optionsRaw, optionsRaw + optionsLen, back_inserter(options));
    }
  } catch (std::out_of_range& e) {
    throw FbossError("DHCPv6 packet parse error: too small packet");
  }
}

size_t DHCPv6Packet::computePacketLength() const {
  if (isDHCPv6Relay()) {
    // type + hopcount + la + pa + options
    return TYPE_BYTES + HOPCOUNT_BYTES + LINKADDR_BYTES + PEERADDR_BYTES +
        options.size();
  } else {
    // type + transaction id + options
    return TYPE_BYTES + TRANSACTIONID_BYTES + options.size();
  }
}

/**
 * Extract options based on the optionSelector vector;
 * If the optionSelect is empty, this function will extract all options;
 * Otherwise, it will extract options that matches the types in
 * the option selector.
 */
std::vector<DHCPv6Option> DHCPv6Packet::extractOptions(
    std::unordered_set<uint16_t> optionSelector) {
  int i = 0;
  std::vector<DHCPv6Option> parsedOptions;
  while (i < options.size() - 4) {
    DHCPv6Option opt;
    opt.parse(options, i);
    if (optionSelector.empty() || optionSelector.count(opt.op) > 0) {
      parsedOptions.push_back(opt);
    }
    i += (opt.len + 4);
  }
  return parsedOptions;
}

string DHCPv6Packet::toString() const {
  stringstream ss;
  if (isDHCPv6Relay()) {
    ss << "DHCPv6Packet (relay) { type: " << (int)type
       << " hopCount: " << (int)hopCount << " linkAddr: " << linkAddr.str()
       << " peerAddr: " << peerAddr.str() << " option len: " << options.size()
       << " }";
  } else {
    ss << "DHCPv6Packet { type: " << (int)type
       << " transaction id: " << transactionId
       << " option len: " << options.size() << " }";
  }
  return ss.str();
}

bool DHCPv6Packet::operator==(const DHCPv6Packet& r) const {
  if (isDHCPv6Relay()) {
    return type == r.type && hopCount == r.hopCount && linkAddr == r.linkAddr &&
        peerAddr == r.peerAddr && options == r.options;
  } else {
    return type == r.type && transactionId == r.transactionId &&
        options == r.options;
  }
}

} // namespace facebook::fboss
