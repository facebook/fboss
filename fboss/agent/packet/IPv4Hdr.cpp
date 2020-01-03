/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/IPv4Hdr.h"

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include "fboss/agent/packet/PktUtil.h"
using std::string;
using std::stringstream;

namespace facebook::fboss {

using folly::IOBuf;
using folly::IPAddressV4;
using folly::io::Appender;
using folly::io::Cursor;

IPv4Hdr::IPv4Hdr(Cursor& cursor) {
  try {
    uint8_t buf[12];
    cursor.pull(buf, sizeof(buf));
    version = buf[0] >> 4;
    if (version != IPV4_VERSION) {
      throw HdrParseError("IPv4: version != 4");
    }
    ihl = buf[0] & 0x0F;
    if (ihl < 5) {
      throw HdrParseError("IPv4: IHL < 5");
    }
    dscp = buf[1] >> 2;
    ecn = buf[1] & 0x03;
    length =
        (static_cast<uint16_t>(buf[2]) << 8) | static_cast<uint16_t>(buf[3]);
    if (length < size()) {
      throw HdrParseError("IPv4: total length < ihl * 4");
    }
    id = (static_cast<uint16_t>(buf[4]) << 8) | static_cast<uint16_t>(buf[5]);
    dontFragment = static_cast<bool>(buf[6] >> 6);
    moreFragments = static_cast<bool>((buf[6] & 0x20) >> 5);
    fragmentOffset = (static_cast<uint16_t>(buf[6] & 0x1F) << 8) |
        static_cast<uint16_t>(buf[7]);
    ttl = buf[8];
    if (ttl == 0) {
      throw HdrParseError("IPv4: TTL == 0");
    }
    protocol = buf[9];
    csum =
        (static_cast<uint16_t>(buf[10]) << 8) | static_cast<uint16_t>(buf[11]);
    // TODO: check the checksum
    uint32_t srcAddrRaw = cursor.readBE<uint32_t>();
    uint32_t dstAddrRaw = cursor.readBE<uint32_t>();
    srcAddr = IPAddressV4::fromLongHBO(srcAddrRaw);
    dstAddr = IPAddressV4::fromLongHBO(dstAddrRaw);

    if (UNLIKELY(ihl > 5)) {
      cursor.pull(optionBuf, (ihl - 5) * sizeof(uint32_t));
    }

  } catch (const std::out_of_range& e) {
    throw HdrParseError("IPv4 header too small");
  }
}

IPv4Hdr::IPv4Hdr(
    const IPAddressV4& _srcAddr,
    const IPAddressV4& _dstAddr,
    uint8_t _protocol,
    uint16_t _bodyLength) {
  version = IPV4_VERSION;
  ihl = 5;
  dscp = 0;
  ecn = 0;
  length = _bodyLength + IPv4Hdr::minSize();
  id = 0;
  dontFragment = false;
  moreFragments = false;
  fragmentOffset = 0;
  ttl = 255;
  protocol = _protocol;
  csum = 0;
  srcAddr = _srcAddr;
  dstAddr = _dstAddr;
}

void IPv4Hdr::computeChecksum() {
  csum = 0;
  uint8_t buffArr[size()];
  auto buf = IOBuf::wrapBuffer(buffArr, size());
  buf->clear();
  Appender appender(buf.get(), 0);
  write(&appender);
  csum = PktUtil::internetChecksum(buf->data(), size());
}

uint32_t IPv4Hdr::pseudoHdrPartialCsum() const {
  return pseudoHdrPartialCsum(length - (ihl * 4));
}

uint32_t IPv4Hdr::pseudoHdrPartialCsum(uint32_t payloadLength) const {
  uint32_t sum = 0;
  sum += addrPartialCsum(srcAddr);
  sum += addrPartialCsum(dstAddr);
  sum += protocol;
  sum += payloadLength;
  return sum;
}

string IPv4Hdr::toString() const {
  stringstream ss;
  ss << " Version : " << (int)version << " Ihl : " << (int)ihl
     << " Dscp : " << (int)dscp << " Ecn : " << (int)ecn
     << " Length : " << length << " Id : " << id
     << " DontFragment : " << (dontFragment ? "True" : "False")
     << " MoreFragments : " << (moreFragments ? "True" : "False")
     << " FragmentOffset : " << fragmentOffset << " Ttl :" << (int)ttl
     << " Protocol : "
     << "0x" << std::hex << (int)protocol << std::dec << " Csum :" << csum
     << " SrcAddr :" << srcAddr << " DstAddr :" << dstAddr;
  return ss.str();
}

uint32_t IPv4Hdr::addrPartialCsum(const IPAddressV4& addr) {
  IOBuf buf(IOBuf::WRAP_BUFFER, addr.bytes(), 4);
  Cursor c(&buf);
  return c.readBE<uint16_t>() + c.readBE<uint16_t>();
}

} // namespace facebook::fboss
