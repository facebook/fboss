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

#include <string>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>

namespace folly {
class IOBuf;
namespace io {
class Appender;
class Cursor;
} // namespace io
} // namespace folly

namespace facebook::fboss {

class PktUtil {
 public:
  /**
   * Read a MacAddress from a Cursor pointing into this packet's buffer.
   *
   * The cursor will be updated to point just past the MacAddress when this
   * function returns.
   */
  static folly::MacAddress readMac(folly::io::Cursor* cursor);

  /**
   * Read an IPv4 adddress from a Cursor pointing into this packet's buffer.
   *
   * The cursor will be updated to point just past the IP address when this
   * function returns.
   */
  static folly::IPAddressV4 readIPv4(folly::io::Cursor* cursor);

  /**
   * Read an IPv6 adddress from a Cursor pointing into this packet's buffer.
   *
   * The cursor will be updated to point just past the IP address when this
   * function returns.
   */
  static folly::IPAddressV6 readIPv6(folly::io::Cursor* cursor);

  /*
   * Compute internet checksum (as defined in RFC 1071) over a sequence
   * of bytes. Code is as given in RFC 1071 section 4.1, with some
   * modifications to handle odd number of bytes regardless of
   * host m/c byte order.
   * Checksum is returned in host byte order. Make sure to convert it
   * to n/w byte order if before sending it out on a wire.
   */
  static uint16_t internetChecksum(folly::io::Cursor start, uint64_t length);
  static uint16_t internetChecksum(const uint8_t* buffer, uint32_t size);
  static uint16_t internetChecksum(const folly::IOBuf* buf);

  /*
   * Incrementally compute an internet checksum.
   *
   * start and length specify the range to checksum.
   * partialChecksum() requires an even number of bytes to checksum.
   * If the packet ends with an odd number of bytes, finalizeChecksum() may be
   * used to compute the checksum over the final odd-length piece.
   *
   * value specifies the initial partial checksum value.  This is normally the
   * value returned by a previous call to partialChecksum().
   *
   * Once all of the data has been passed to partialChecksum(),
   * finalizeChecksum() should be called to get the final checksum value.
   */
  static uint32_t
  partialChecksum(folly::io::Cursor start, uint64_t length, uint32_t value = 0);
  static uint16_t
  finalizeChecksum(folly::io::Cursor start, uint64_t length, uint32_t value);
  static uint16_t finalizeChecksum(uint32_t value);

  /**
   * Return a string containing a human readable hex dump of the binary data.
   */
  static std::string hexDump(folly::io::Cursor cursor);
  static std::string hexDump(folly::io::Cursor start, folly::io::Cursor end);
  static std::string hexDump(folly::io::Cursor start, uint32_t length);

  /*
   * Parse an ASCII hex string and return the binary data in a folly::IOBuf.
   */
  static folly::IOBuf parseHexData(folly::StringPiece hex);

  /*
   * Parse an ASCII hex string, and write the binary data to the specified
   * Appender.
   */
  static void appendHexData(
      folly::StringPiece hex,
      folly::io::Appender* appender);

  static void padToLength(folly::IOBuf* buf, uint32_t size, uint8_t pad = 0);

 private:
  // Forbidden copy constructor and assignment operator
  PktUtil(PktUtil const&) = delete;
  PktUtil& operator=(PktUtil const&) = delete;

  static uint32_t
  partialChecksumImpl(folly::io::Cursor start, uint64_t length, uint32_t value);
};

} // namespace facebook::fboss
