/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/DHCPv4Packet.h"
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include "fboss/agent/DHCPv4OptionsOfInterest.h"
#include "fboss/agent/FbossError.h"

using folly::IPAddressV4;
using folly::io::Cursor;
using std::back_inserter;
using std::copy;
using std::vector;

namespace facebook::fboss {

class SwSwitch;
const uint8_t DHCPv4Packet::kOptionsCookie[] = {99, 130, 83, 99};

bool DHCPv4Packet::isOptionWithoutLength(uint8_t op) {
  return op == DHCPv4OptionsOfInterest::PAD ||
      op == DHCPv4OptionsOfInterest::END;
}

void DHCPv4Packet::parse(Cursor* cursor) {
  try {
    op = cursor->read<uint8_t>();
    htype = cursor->read<uint8_t>();
    hlen = cursor->read<uint8_t>();
    hops = cursor->read<uint8_t>();
    xid = IPAddressV4::fromLong(cursor->read<uint32_t>());
    secs = cursor->readBE<uint16_t>();
    flags = cursor->readBE<uint16_t>();
    ciaddr = IPAddressV4::fromLong(cursor->read<uint32_t>());
    yiaddr = IPAddressV4::fromLong(cursor->read<uint32_t>());
    siaddr = IPAddressV4::fromLong(cursor->read<uint32_t>());
    giaddr = IPAddressV4::fromLong(cursor->read<uint32_t>());
    cursor->pull(chaddr.data(), chaddr.size());
    cursor->pull(sname.data(), sname.size());
    cursor->pull(file.data(), file.size());
    uint8_t cookie[kOptionsCookieSize];
    cursor->pull(cookie, kOptionsCookieSize);
    if (memcmp(cookie, kOptionsCookie, kOptionsCookieSize)) {
      // BOOTP Packet copy bytes back to options
      copy(cookie, cookie + 4, back_inserter(options));
    } else {
      // DHCP Packet
      copy(cookie, cookie + kOptionsCookieSize, back_inserter(dhcpCookie));
      CHECK(dhcpCookie.size() == kOptionsCookieSize);
    }
    if (cursor->totalLength()) {
      auto optionsLen = cursor->totalLength();
      uint8_t optionsRaw[optionsLen];
      options.reserve(options.size() + optionsLen);
      cursor->pullAtMost(optionsRaw, optionsLen);
      copy(optionsRaw, optionsRaw + optionsLen, back_inserter(options));
    }
  } catch (std::out_of_range& e) {
    throw FbossError(
        "Too small packet, "
        "expected minimum ",
        minSize(),
        " bytes");
  }
}

bool DHCPv4Packet::operator==(const DHCPv4Packet& r) const {
  return op == r.op && htype == r.htype && hlen == r.hlen && hops == r.hops &&
      xid == r.xid && secs == r.secs && flags == r.flags &&
      ciaddr == r.ciaddr && yiaddr == r.yiaddr && siaddr == r.siaddr &&
      giaddr == r.giaddr && chaddr == r.chaddr && sname == r.sname &&
      file == r.file && dhcpCookie == r.dhcpCookie && options == r.options;
}

size_t
DHCPv4Packet::appendOption(uint8_t op, uint8_t len, const uint8_t* bytes) {
  XLOG(DBG4) << "Appending option: " << (int)op << " of length: " << (int)len;

  if (isOptionWithoutLength(op)) {
    options.push_back(op);
    return 1;
  }
  auto origSize = options.size();
  options.reserve(options.size() + 2 + len);
  options.push_back(op);
  options.push_back(len);
  copy(bytes, bytes + len, std::back_inserter(options));
  CHECK(options.size() == origSize + 2 + len);
  return 2 + len;
}

void DHCPv4Packet::appendPadding(size_t length) {
  options.insert(options.end(), length, DHCPv4OptionsOfInterest::PAD);
}

void DHCPv4Packet::padToMinLength() {
  auto curSize = size();
  if (curSize < kMinSize) {
    appendPadding(kMinSize - curSize);
  }
}

bool DHCPv4Packet::getOptionSlow(
    uint8_t op,
    const Options& options,
    vector<uint8_t>& optionData) {
  auto optIndex = 0;
  while (optIndex < options.size()) {
    uint8_t curOp = options[optIndex];
    uint8_t opDataLen =
        isOptionWithoutLength(curOp) ? 0 : options[optIndex + 1];
    if (op == curOp) {
      if (opDataLen) {
        copy(
            &options[optIndex + 2],
            &options[optIndex + 2 + opDataLen],
            std::back_inserter(optionData));
      }
      return true;
    }
    optIndex += opDataLen == 0 ? 1 : 2 + opDataLen;
  }
  return false;
}

} // namespace facebook::fboss
