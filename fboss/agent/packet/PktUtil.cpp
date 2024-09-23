/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/PktUtil.h"

#include <folly/Format.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include "fboss/agent/FbossError.h"

using folly::ByteRange;
using folly::IOBuf;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::StringPiece;
using folly::io::Appender;
using folly::io::Cursor;
using std::string;

namespace facebook::fboss {

MacAddress PktUtil::readMac(Cursor* cursor) {
  // Common case is that the MAC data is contiguous
  if (cursor->length() >= MacAddress::SIZE) {
    const auto* data = cursor->data();
    cursor->skip(MacAddress::SIZE);
    return MacAddress::fromBinary(folly::ByteRange(data, MacAddress::SIZE));
  }

  // Copy to a temporary buffer to handle the non-contiguous case.
  uint8_t buf[MacAddress::SIZE];
  cursor->pull(buf, MacAddress::SIZE);
  return MacAddress::fromBinary(folly::ByteRange(buf, MacAddress::SIZE));
}

IPAddressV4 PktUtil::readIPv4(Cursor* cursor) {
  // Note that IPAddressV4 accepts a network-byte order uint32_t,
  // so we use cursor->read() rather than cursor->readBE().
  return IPAddressV4::fromLong(cursor->read<uint32_t>());
}

IPAddressV6 PktUtil::readIPv6(Cursor* cursor) {
  enum { IPV6_LENGTH = IPAddressV6::byteCount() };
  // Common case is that the IP address data is contiguous
  if (cursor->length() >= IPV6_LENGTH) {
    const auto* data = cursor->data();
    cursor->skip(IPV6_LENGTH);
    return IPAddressV6::fromBinary(ByteRange(data, IPV6_LENGTH));
  }

  // Copy to a temporary buffer to handle the non-contiguous case.
  uint8_t buf[IPV6_LENGTH];
  cursor->pull(buf, IPV6_LENGTH);
  return IPAddressV6::fromBinary(ByteRange(buf, IPV6_LENGTH));
}

uint16_t PktUtil::internetChecksum(folly::io::Cursor start, uint64_t length) {
  return finalizeChecksum(start, length, 0);
}

uint16_t PktUtil::internetChecksum(const uint8_t* buffer, uint32_t size) {
  auto buf = IOBuf(IOBuf::WRAP_BUFFER, buffer, size);
  return finalizeChecksum(Cursor(&buf), size, 0);
}

uint16_t PktUtil::internetChecksum(const IOBuf* buf) {
  return finalizeChecksum(Cursor(buf), buf->computeChainDataLength(), 0);
}

uint32_t PktUtil::partialChecksumImpl(
    folly::io::Cursor cursor,
    uint64_t length,
    uint32_t value) {
  // Checksum all the pairs of bytes first
  while (length > 1) {
    value += cursor.readBE<uint16_t>();
    length -= 2;
  }
  if (length) {
    // Bytes are interpreted in n/w byte order
    // so this last octet represents the 8 MSbits
    // of the last 16 bit number in this buffer.
    uint16_t last = cursor.read<uint8_t>();
    value += (last << 8);
    --length;
  }
  return value;
}

uint32_t PktUtil::partialChecksum(
    folly::io::Cursor cursor,
    uint64_t length,
    uint32_t value) {
  CHECK((length & 1) == 0);
  return partialChecksumImpl(cursor, length, value);
}

uint16_t PktUtil::finalizeChecksum(
    folly::io::Cursor start,
    uint64_t length,
    uint32_t value) {
  auto sum = partialChecksumImpl(start, length, value);
  return finalizeChecksum(sum);
}

uint16_t PktUtil::finalizeChecksum(uint32_t sum) {
  // Add carry.
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  sum = ~sum;
  return static_cast<uint16_t>(sum);
}

string PktUtil::hexDump(Cursor cursor) {
  return hexDump(cursor, cursor.totalLength());
}

string PktUtil::hexDump(Cursor start, Cursor end) {
  return hexDump(start, end - start);
}

string PktUtil::hexDump(Cursor cursor, uint32_t length) {
  if (UNLIKELY(length == 0)) {
    return "";
  }

  // Go ahead and reserve the required space
  size_t numLines = (length + 15) / 16;
  size_t expectedLength = (length * 3) + // 3 bytes for each character
      (numLines * 6); // 5 bytes for each line prefix plus 1 for newline

  string result;
  result.reserve(expectedLength);
  for (size_t n = 0; n < length; ++n) {
    if (n == 0) {
      result.append("0000:");
    } else if ((n & 0xf) == 0) {
      folly::format(&result, "\n{:04x}:", n);
    }
    folly::format(&result, " {:02x}", cursor.read<uint8_t>());
  }

  DCHECK_EQ(result.size() + 1, expectedLength);
  return result;
}

string PktUtil::hexDump(const IOBuf* buf) {
  Cursor cursor(buf);
  return hexDump(cursor);
}

IOBuf PktUtil::parseHexData(StringPiece hex) {
  // The hex encoded version is at least 2 bytes per binary byte.
  // Allocating hex.size() / 2 will always be more room than we need.
  IOBuf buf(IOBuf::CREATE, hex.size() / 2);
  Appender a(&buf, 4096);
  appendHexData(hex, &a);
  return buf;
}

void PktUtil::appendHexData(StringPiece hex, Appender* appender) {
  auto unhex = [](char c) -> int {
    return c >= '0' && c <= '9' ? c - '0'
        : c >= 'A' && c <= 'F'  ? c - 'A' + 10
        : c >= 'a' && c <= 'f'  ? c - 'a' + 10
                                : -1;
  };

  int prev = -1;
  for (size_t idx = 0; idx < hex.size(); ++idx) {
    char c = hex[idx];
    // Skip over whitespace between bytes
    if (isspace(c, std::locale("C"))) {
      if (prev != -1) {
        throw FbossError(
            "invalid hex at offset ", idx, ": each hex byte must be 2 digits");
      }
      continue;
    }

    int value = unhex(c);
    if (value < 0) {
      throw FbossError("invalid hex digit '", c, "' at offset ", idx);
    }
    if (prev == -1) {
      prev = value;
    } else {
      uint8_t byte = (prev << 4) | value;
      appender->write(byte);
      prev = -1;
    }
  }
}

void PktUtil::padToLength(folly::IOBuf* buf, uint32_t size, uint8_t pad) {
  auto len = buf->computeChainDataLength();
  if (len >= size) {
    return;
  }

  size_t toPad = size - len;
  Appender app(buf, 1024);
  app.ensure(toPad);
  memset(app.writableData(), pad, toPad);
  app.append(toPad);
}

} // namespace facebook::fboss
