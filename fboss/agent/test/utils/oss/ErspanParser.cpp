/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/ErspanParser.h"

#include <array>
#include <stdexcept>

#include <fmt/format.h>
#include <folly/Range.h>

namespace facebook::fboss::utility {

namespace {
// GRE base header: 2-byte flags/version + 2-byte protocol type.
constexpr uint8_t kGreHeaderBytes = 4;
// ETH-ERSPAN Mirror Header Version 1 (NVIDIA Switch PRM Table 573): the fixed
// portion (excluding the optional pad_count trailing bytes) between the GRE
// header and the mirrored L2 frame.
//   00h [ingress_label_port:16][raw_opcode:8][pad_count:8]
//   04h [flags:8][packet_type:2|reserved:6][timestamp[79:64]:16]
//   08h/0Ch timestamp[63:32] / timestamp[31:0]
//   10h [original_packet_size:16][egress_label_port:16]
//   14h [psn:16][padding: pad_count bytes]
// Bytes between pad_count and original_packet_size: flags(1)+packet_type(1)+
// timestamp(10).
constexpr uint8_t kFlagsToTimestampBytes = 12;
constexpr uint8_t kEthMacBytes = 12; // dst(6) + src(6)
constexpr uint16_t kEtherTypeIPv4 = 0x0800;
constexpr uint16_t kEtherTypeIPv6 = 0x86dd;
constexpr uint16_t kEtherTypeVlan = 0x8100; // 802.1Q tag
constexpr uint8_t kIpv4AddrBytes = 4;
constexpr uint8_t kIpv6AddrBytes = 16;
} // namespace

// Open-source implementation: parse the Chenab ETH-ERSPAN Mirror Header
// Version 1 directly, without the (non-open-source) nettools sFlow parser.
ErspanSampleInfo parseChenabErspanSample(folly::io::Cursor& cursor) {
  ErspanSampleInfo info;

  cursor.skip(kGreHeaderBytes);
  info.ingressLabelPort = cursor.readBE<uint16_t>();
  cursor.skip(sizeof(uint8_t)); // raw_opcode
  const uint8_t padCount = cursor.readBE<uint8_t>();
  cursor.skip(kFlagsToTimestampBytes);
  info.originalPacketSize = cursor.readBE<uint16_t>();
  info.egressLabelPort = cursor.readBE<uint16_t>();
  cursor.skip(sizeof(uint16_t)); // psn
  cursor.skip(padCount); // optional trailing padding

  // Mirrored inner Ethernet frame. The test exercises IPv6, but IPv4 is parsed
  // too (below) for parity with the internal nettools parser.
  cursor.skip(kEthMacBytes);
  uint16_t etherType = cursor.readBE<uint16_t>();
  // Skip any 802.1Q VLAN tag(s) so a tagged mirrored frame decodes the same way
  // the nettools parser does (it skips the tag before reading the L3 type).
  while (etherType == kEtherTypeVlan) {
    cursor.skip(sizeof(uint16_t)); // VLAN TCI
    etherType = cursor.readBE<uint16_t>();
  }
  // Parse the inner L3 header. The internal (nettools) parser handles both IPv6
  // and IPv4; match it here so the two implementations stay observably
  // equivalent, and fail loudly on anything else rather than returning
  // default-constructed fields that surface as a confusing EXPECT mismatch.
  if (etherType == kEtherTypeIPv6) {
    cursor.skip(4); // version / traffic class / flow label
    cursor.skip(2); // payload length
    info.ipProtocol = cursor.readBE<uint8_t>(); // next header
    cursor.skip(1); // hop limit
    std::array<uint8_t, kIpv6AddrBytes> src{};
    std::array<uint8_t, kIpv6AddrBytes> dst{};
    cursor.pull(src.data(), src.size());
    cursor.pull(dst.data(), dst.size());
    info.srcAddr =
        folly::IPAddress::fromBinary(folly::ByteRange(src.data(), src.size()));
    info.dstAddr =
        folly::IPAddress::fromBinary(folly::ByteRange(dst.data(), dst.size()));
  } else if (etherType == kEtherTypeIPv4) {
    const uint8_t versionIhl = cursor.readBE<uint8_t>();
    const uint8_t ihlBytes = (versionIhl & 0x0F) * 4;
    cursor.skip(8); // dscp/ecn(1)+total_len(2)+id(2)+flags/frag(2)+ttl(1)
    info.ipProtocol = cursor.readBE<uint8_t>(); // protocol
    cursor.skip(2); // header checksum
    std::array<uint8_t, kIpv4AddrBytes> src{};
    std::array<uint8_t, kIpv4AddrBytes> dst{};
    cursor.pull(src.data(), src.size());
    cursor.pull(dst.data(), dst.size());
    info.srcAddr =
        folly::IPAddress::fromBinary(folly::ByteRange(src.data(), src.size()));
    info.dstAddr =
        folly::IPAddress::fromBinary(folly::ByteRange(dst.data(), dst.size()));
    if (ihlBytes > 20) {
      cursor.skip(ihlBytes - 20); // IPv4 options before the L4 header
    }
  } else {
    throw std::runtime_error(
        fmt::format(
            "Chenab ERSPAN OSS parser: unsupported inner ethertype 0x{:04x}",
            etherType));
  }
  // TCP and UDP both begin with 16-bit source and destination ports.
  info.l4SrcPort = cursor.readBE<uint16_t>();
  info.l4DstPort = cursor.readBE<uint16_t>();
  return info;
}

} // namespace facebook::fboss::utility
