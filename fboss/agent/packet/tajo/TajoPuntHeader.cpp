/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/tajo/TajoPuntHeader.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <vector>

// Parses the Tajo (Cisco Silicon One) punt header from a captured packet.
//
// The SDK ships an authoritative parser
// (silicon_one::la_header_parser::parse_punt_header), but its symbols live in
// device-family-specific libraries (libheader_parser_{g2ll,gibraltar,gr2}.so)
// that are not exposed as buck targets, so we cannot link them here. Instead we
// parse the fixed 40-byte punt header directly off the wire via the
// LaPacketPuntHeader bitfield union below. If the wire layout ever diverges for
// a device family, the fix is to buckify libheader_parser_* and call the SDK
// parser instead.

namespace facebook::fboss {

namespace {

#pragma pack(push, 1)
union LaPacketPuntHeader {
  struct {
    uint64_t future_use : 64;
    uint64_t fwd_qos_tag : 7;
    uint64_t qos_group : 7;
    uint64_t encap_qos_tag : 7;
    uint64_t padding1 : 7;
    uint64_t version : 4;
    uint64_t receive_time : 32;
    uint64_t time_stamp : 64;
    uint64_t relay_id : 14;
    uint64_t padding2 : 2;
    uint64_t destination_lp : 20;
    uint64_t source_lp : 20;
    uint64_t destination_sp : 16;
    uint64_t source_sp : 16;
    uint64_t lpts_flow_type : 8;
    uint64_t code : 8;
    uint64_t source : 4;
    uint64_t next_header_offset : 8;
    uint64_t tc : 3;
    uint64_t fwd_header_type : 4;
    uint64_t next_header : 5;
  } fields;
  uint8_t raw[kTajoNplPuntHeaderBytes];
};
#pragma pack(pop)

// Assert on the whole union, not on `raw` (a uint8_t[N], trivially N bytes).
// The union is as large as the bitfield struct, so this fails if the compiler
// pads the bitfields past the wire size -- which would silently misalign every
// parsed field.
static_assert(
    sizeof(LaPacketPuntHeader) == kTajoNplPuntHeaderBytes,
    "punt header bitfield layout must be exactly kTajoNplPuntHeaderBytes");

std::vector<uint8_t> peekBytes(folly::io::Cursor cursor, size_t numBytes) {
  std::vector<uint8_t> raw;
  raw.reserve(numBytes);
  for (size_t i = 0; i < numBytes; ++i) {
    if (!cursor.canAdvance(1)) {
      break;
    }
    raw.push_back(cursor.read<uint8_t>());
  }
  return raw;
}

TajoPuntNextHeaderKind mapKernelNextHeaderKind(uint64_t nextHeader) {
  switch (nextHeader) {
    case 1: // FPP_PROTOCOL_ETHERNET
      return TajoPuntNextHeaderKind::Ethernet;
    case 4: // FPP_PROTOCOL_IPV4
      return TajoPuntNextHeaderKind::Ipv4;
    case 6: // FPP_PROTOCOL_IPV6
      return TajoPuntNextHeaderKind::Ipv6;
    default:
      return TajoPuntNextHeaderKind::Unknown;
  }
}

void setIngressPort(ParsedTajoPuntHeader& out) {
  // ingressPort is a 16-bit port. Prefer the 16-bit source system port; fall
  // back to the 20-bit logical port only when it fits in 16 bits, so a large
  // logical-port value is never silently truncated into a bogus port.
  if (out.sourceSp != 0) {
    out.ingressPort = out.sourceSp;
  } else if (out.sourceLp != 0 && out.sourceLp <= 0xFFFF) {
    out.ingressPort = static_cast<uint16_t>(out.sourceLp);
  }
}

void populateFromLaPacketFields(
    const LaPacketPuntHeader& punt,
    ParsedTajoPuntHeader& out) {
  const auto& f = punt.fields;
  out.nextHeader = static_cast<uint8_t>(f.next_header);
  out.fwdHeaderType = static_cast<uint8_t>(f.fwd_header_type);
  out.tc = static_cast<uint8_t>(f.tc);
  out.nextHeaderOffset = static_cast<uint8_t>(f.next_header_offset);
  out.puntSource = static_cast<uint8_t>(f.source);
  out.code = static_cast<uint8_t>(f.code);
  out.lptsFlowType = static_cast<uint8_t>(f.lpts_flow_type);
  out.sourceSp = static_cast<uint16_t>(f.source_sp);
  out.destinationSp = static_cast<uint16_t>(f.destination_sp);
  out.sourceLp = static_cast<uint32_t>(f.source_lp);
  out.destinationLp = static_cast<uint32_t>(f.destination_lp);
  out.relayId = static_cast<uint16_t>(f.relay_id);
  out.timeStamp = f.time_stamp;
  out.receiveTime = static_cast<uint32_t>(f.receive_time);
  out.version = static_cast<uint8_t>(f.version);
  out.fwdQosTag = static_cast<uint8_t>(f.fwd_qos_tag);
  out.qosGroup = static_cast<uint8_t>(f.qos_group);
  out.encapQosTag = static_cast<uint8_t>(f.encap_qos_tag);

  out.nextHeaderKind = mapKernelNextHeaderKind(f.next_header);
  out.dropReason = out.code;
  setIngressPort(out);
}

std::optional<ParsedTajoPuntHeader> tryParseWirePuntHeader(
    const std::vector<uint8_t>& puntBytes) {
  if (puntBytes.size() != kTajoNplPuntHeaderBytes) {
    return std::nullopt;
  }

  LaPacketPuntHeader punt{};
  std::memcpy(punt.raw, puntBytes.data(), kTajoNplPuntHeaderBytes);

  if (mapKernelNextHeaderKind(punt.fields.next_header) ==
      TajoPuntNextHeaderKind::Unknown) {
    return std::nullopt;
  }

  // Disambiguate the reversed (SDK) vs raw wire byte order: next_header is only
  // 5 bits, so a wrong byte order can coincidentally decode to a recognized
  // value. On a correctly-ordered header the reserved padding bits are zero, so
  // require that too — this prevents a spuriously-parsing order from being
  // preferred over the well-formed one, which would silently misread every
  // other field.
  if (punt.fields.padding1 != 0 || punt.fields.padding2 != 0) {
    return std::nullopt;
  }

  ParsedTajoPuntHeader out;
  out.parsedByWire = true;
  out.consumedBytes = static_cast<uint32_t>(kTajoNplPuntHeaderBytes);
  populateFromLaPacketFields(punt, out);
  return out;
}

// The wire bitfield layout depends on compiler packing and byte order, so try
// the SDK byte order (reversed) first and fall back to raw wire order.
std::optional<ParsedTajoPuntHeader> tryParseWirePuntHeaderFromCursor(
    folly::io::Cursor cursor) {
  auto wireOrder = peekBytes(cursor, kTajoNplPuntHeaderBytes);
  if (wireOrder.size() != kTajoNplPuntHeaderBytes) {
    return std::nullopt;
  }

  auto sdkOrder = wireOrder;
  std::reverse(sdkOrder.begin(), sdkOrder.end());

  if (auto parsed = tryParseWirePuntHeader(sdkOrder)) {
    return parsed;
  }
  return tryParseWirePuntHeader(wireOrder);
}

} // namespace

ParsedTajoPuntHeader parseTajoPuntHeader(folly::io::Cursor& cursor) {
  if (auto parsed = tryParseWirePuntHeaderFromCursor(cursor)) {
    cursor.skip(parsed->consumedBytes);
    return *parsed;
  }

  // Unrecognized header: skip the fixed punt-header size, but only if the
  // cursor actually has that many bytes -- a short/malformed buffer would
  // otherwise make skip() throw. Leave consumedBytes at 0 in that case.
  ParsedTajoPuntHeader out;
  if (!cursor.canAdvance(kTajoNplPuntHeaderBytes)) {
    return out;
  }
  out.consumedBytes = static_cast<uint32_t>(kTajoNplPuntHeaderBytes);
  cursor.skip(kTajoNplPuntHeaderBytes);
  return out;
}

} // namespace facebook::fboss
