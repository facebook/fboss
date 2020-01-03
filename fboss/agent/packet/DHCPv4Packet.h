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
#include <folly/IPAddressV4.h>
#include <array>
#include <memory>
#include <vector>

namespace folly {
namespace io {
class Cursor;
}
} // namespace folly

namespace facebook::fboss {

class SwSwitch;

struct DHCPv4Packet {
 public:
  DHCPv4Packet() {}
  typedef std::array<uint8_t, 16> Chaddr;
  typedef std::array<uint8_t, 64> Sname;
  typedef std::array<uint8_t, 128> File;
  typedef std::vector<uint8_t> DhcpCookie;
  typedef std::vector<uint8_t> Options;
  enum : uint8_t { kOptionsCookieSize = 4 };
  enum : uint16_t { kFlagBroadcast = 0x8000 };
  enum : size_t { kFixedPartBytes = 236 };
  enum : size_t { kMinSize = 300 };
  static const uint8_t kOptionsCookie[kOptionsCookieSize];

  void parse(folly::io::Cursor* cursor);

  bool operator==(const DHCPv4Packet& r) const;

  size_t size() const {
    return kFixedPartBytes + dhcpCookie.size() + options.size();
  }
  bool hasOptions() const {
    return options.size();
  }

  bool hasDhcpCookie() const {
    return dhcpCookie.size();
  }

  void clearOptions() {
    options.clear();
  }
  static constexpr size_t minSize() {
    return kFixedPartBytes + kOptionsCookieSize;
  }
  static bool isOptionWithoutLength(uint8_t op);

  size_t appendOption(uint8_t op, uint8_t len, const uint8_t* bytes);

  void appendPadding(size_t length);

  /*
   * Pad this packet to the minimum required length (300 bytes).
   */
  void padToMinLength();

  /*
   * Linear time traversal of options vector to fund a given option
   * Function returns true if the option is found and if the option
   * has a non zero length, fills in the option data in optionData
   * vector.
   */
  static bool getOptionSlow(
      uint8_t op,
      const Options& options,
      std::vector<uint8_t>& optionData);
  template <typename CursorType>
  void write(CursorType* cursor) const;
  /* From rfc 2131
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
    +---------------+---------------+---------------+---------------+
    |                            xid (4)                            |
    +-------------------------------+-------------------------------+
    |           secs (2)            |           flags (2)           |
    +-------------------------------+-------------------------------+
    |                          ciaddr  (4)                          |
    +---------------------------------------------------------------+
    |                          yiaddr  (4)                          |
    +---------------------------------------------------------------+
    |                          siaddr  (4)                          |
    +---------------------------------------------------------------+
    |                          giaddr  (4)                          |
    +---------------------------------------------------------------+
    |                                                               |
    |                          chaddr  (16)                         |
    |                                                               |
    |                                                               |
    +---------------------------------------------------------------+
    |                                                               |
    |                          sname   (64)                         |
    +---------------------------------------------------------------+
    |                                                               |
    |                          file    (128)                        |
    +---------------------------------------------------------------+
    |                                                               |
    |                          options (variable)                   |
    +---------------------------------------------------------------+
   */
  uint8_t op;
  uint8_t htype;
  uint8_t hlen;
  uint8_t hops;
  folly::IPAddressV4 xid;
  uint16_t secs;
  uint16_t flags;
  folly::IPAddressV4 ciaddr;
  folly::IPAddressV4 yiaddr;
  folly::IPAddressV4 siaddr;
  folly::IPAddressV4 giaddr;
  Chaddr chaddr;
  Sname sname;
  File file;
  // DHCP cookie is present for DHCP packet but not
  // for BOOTP packets
  std::vector<uint8_t> dhcpCookie;
  std::vector<uint8_t> options;
};

template <typename CursorType>
void DHCPv4Packet::write(CursorType* cursor) const {
  cursor->template write<uint8_t>(op);
  cursor->template write<uint8_t>(htype);
  cursor->template write<uint8_t>(hlen);
  cursor->template write<uint8_t>(hops);
  cursor->template write<uint32_t>(xid.toLong());
  cursor->template writeBE<uint16_t>(secs);
  cursor->template writeBE<uint16_t>(flags);
  cursor->template write<uint32_t>(ciaddr.toLong());
  cursor->template write<uint32_t>(yiaddr.toLong());
  cursor->template write<uint32_t>(siaddr.toLong());
  cursor->template write<uint32_t>(giaddr.toLong());
  cursor->push(chaddr.data(), chaddr.size());
  cursor->push(sname.data(), sname.size());
  cursor->push(file.data(), file.size());
  // Push DHCP cookie
  if (hasDhcpCookie()) {
    cursor->push(&dhcpCookie[0], dhcpCookie.size());
  }
  // Push options
  if (hasOptions()) {
    cursor->push(&options[0], options.size());
  }
}

} // namespace facebook::fboss
