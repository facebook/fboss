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

#include <cstddef>
#include <cstdint>

#include <folly/io/Cursor.h>

namespace facebook::fboss {

// Parser for the Cisco Silicon One (Tajo) punt header -- the fixed-size
// metadata header the ASIC prepends to punted/mirrored packets. On a Tajo
// Mirror-on-Drop export packet the wire layout is:
//   outer Eth / outer IPv6 / outer UDP / punt header / [ENE dummy] / inner
// This file parses the punt header and provides geometry helpers for the
// surrounding MoD layers; the outer/inner layers are parsed by the caller with
// the standard EthHdr/IPv6Hdr/... headers.

enum class TajoPuntNextHeaderKind {
  Unknown,
  Ethernet,
  Ipv4,
  Ipv6,
};

constexpr size_t kTajoModOuterEthLen = 14;
constexpr size_t kTajoModOuterIpv6Len = 40;
constexpr size_t kTajoModOuterUdpLen = 8;
constexpr size_t kTajoNplPuntHeaderBytes = 40;
constexpr size_t kTajoModInnerEthLen = 14;

// Bytes of the mirrored inner sample after the punt header on the wire.
inline size_t tajoModInnerBytesOnWire(
    size_t frameLen,
    size_t outerEthLen,
    size_t puntConsumedBytes = kTajoNplPuntHeaderBytes) {
  const size_t overhead = outerEthLen + kTajoModOuterIpv6Len +
      kTajoModOuterUdpLen + puntConsumedBytes;
  return frameLen < overhead ? 0 : frameLen - overhead;
}

struct ParsedTajoPuntHeader {
  uint8_t nextHeader{0};
  uint8_t fwdHeaderType{0};
  uint8_t tc{0};
  uint8_t nextHeaderOffset{0};
  uint8_t puntSource{0};
  uint8_t code{0};
  uint8_t lptsFlowType{0};
  uint16_t sourceSp{0};
  uint16_t destinationSp{0};
  uint32_t sourceLp{0};
  uint32_t destinationLp{0};
  uint16_t relayId{0};
  uint64_t timeStamp{0};
  uint32_t receiveTime{0};
  uint8_t version{0};
  uint8_t fwdQosTag{0};
  uint8_t qosGroup{0};
  uint8_t encapQosTag{0};

  uint16_t ingressPort{0};
  uint16_t dropReason{0};
  TajoPuntNextHeaderKind nextHeaderKind{TajoPuntNextHeaderKind::Unknown};
  uint32_t consumedBytes{0};
  // True if the punt header was recognized; false if parsing fell back to
  // skipping the fixed header size (e.g. an unknown next-header value).
  bool parsedByWire{false};
};

inline bool tajoPuntInnerHasEthernetHdr(TajoPuntNextHeaderKind kind) {
  return kind == TajoPuntNextHeaderKind::Ethernet;
}

inline bool tajoPuntInnerStartsAtL3(TajoPuntNextHeaderKind kind) {
  return kind == TajoPuntNextHeaderKind::Ipv4 ||
      kind == TajoPuntNextHeaderKind::Ipv6;
}

// Parse the punt header at the cursor, advancing it past the header.
ParsedTajoPuntHeader parseTajoPuntHeader(folly::io::Cursor& cursor);

} // namespace facebook::fboss
