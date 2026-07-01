// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/TajoPuntHeader.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <vector>

#include "folly/ScopeGuard.h"

#ifndef TAJO_HAS_SDK_PUNT_HEADER_PARSER
#if __has_include(                             \
    "api/header_parser/la_header_parser.h") && \
    __has_include("api/header_parser/la_punt_header.h")
#define TAJO_HAS_SDK_PUNT_HEADER_PARSER 1
#else
#define TAJO_HAS_SDK_PUNT_HEADER_PARSER 0
#endif
#endif

#if TAJO_HAS_SDK_PUNT_HEADER_PARSER
#include "api/header_parser/la_header_parser.h"
#include "api/header_parser/la_punt_header.h"
#endif

namespace facebook::fboss {

namespace {

constexpr size_t kLaPacketPuntHeaderBytes = 40;

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
  uint8_t raw[kLaPacketPuntHeaderBytes];
};
#pragma pack(pop)

static_assert(
    sizeof(LaPacketPuntHeader::raw) == kLaPacketPuntHeaderBytes,
    "punt header raw size");

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
  if (out.sourceSp != 0) {
    out.ingressPort = out.sourceSp;
  } else if (out.sourceLp != 0) {
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
  if (puntBytes.size() != kLaPacketPuntHeaderBytes) {
    return std::nullopt;
  }

  LaPacketPuntHeader punt{};
  std::memcpy(punt.raw, puntBytes.data(), kLaPacketPuntHeaderBytes);

  const auto nextHeaderKind = mapKernelNextHeaderKind(punt.fields.next_header);
  if (nextHeaderKind == TajoPuntNextHeaderKind::Unknown) {
    return std::nullopt;
  }

  ParsedTajoPuntHeader out;
  out.parsedByWire = true;
  out.consumedBytes = static_cast<uint32_t>(kLaPacketPuntHeaderBytes);
  populateFromLaPacketFields(punt, out);
  return out;
}

std::optional<ParsedTajoPuntHeader> tryParseWirePuntHeaderFromCursor(
    folly::io::Cursor cursor) {
  auto wireOrder = peekBytes(cursor, kLaPacketPuntHeaderBytes);
  if (wireOrder.size() != kLaPacketPuntHeaderBytes) {
    return std::nullopt;
  }

  auto sdkOrder = wireOrder;
  std::reverse(sdkOrder.begin(), sdkOrder.end());

  if (auto parsed = tryParseWirePuntHeader(sdkOrder)) {
    return parsed;
  }
  return tryParseWirePuntHeader(wireOrder);
}

#if TAJO_HAS_SDK_PUNT_HEADER_PARSER
TajoPuntNextHeaderKind mapSdkNextHeaderKind(
    silicon_one::la_punt_header_next_header_type_e nextHeader) {
  using Nht = silicon_one::la_punt_header_next_header_type_e;
  switch (nextHeader) {
    case Nht::ETHERNET:
    case Nht::ETHERNET_VLAN:
    case Nht::VLAN_0:
    case Nht::VLAN_1:
      return TajoPuntNextHeaderKind::Ethernet;
    case Nht::IPV4:
    case Nht::IPV4_L4:
    case Nht::IPV4_NON_FIRST_FRAGMENT:
    case Nht::IPV4_FIRST_FRAGMENT:
      return TajoPuntNextHeaderKind::Ipv4;
    case Nht::IPV6:
    case Nht::IPV6_L4:
    case Nht::IPV6_FRAG_EXTENSION:
    case Nht::IPV6_FIRST_FRAGMENT:
    case Nht::SEGMENT_ROUTING:
      return TajoPuntNextHeaderKind::Ipv6;
    default:
      return TajoPuntNextHeaderKind::Unknown;
  }
}

void populateFromSdkHeader(
    const silicon_one::la_punt_header_t& sdk,
    ParsedTajoPuntHeader& out) {
  out.fwdHeaderType = static_cast<uint8_t>(sdk.fwd_header_type);
  if (sdk.tc.is_valid) {
    out.tc = static_cast<uint8_t>(sdk.tc.value);
  }
  if (sdk.ingress_header_offset.is_valid) {
    out.nextHeaderOffset = sdk.ingress_header_offset.value;
  }
  out.puntSource = static_cast<uint8_t>(sdk.punt_source);
  if (sdk.lpts_flow_type.is_valid) {
    out.lptsFlowType = static_cast<uint8_t>(sdk.lpts_flow_type.value);
  }
  if (sdk.ssp_gid.is_valid) {
    out.sourceSp = static_cast<uint16_t>(sdk.ssp_gid.value);
  }
  if (sdk.dsp_gid.is_valid) {
    out.destinationSp = static_cast<uint16_t>(sdk.dsp_gid.value);
  }
  if (sdk.l3_slp_gid.is_valid) {
    out.sourceLp = static_cast<uint32_t>(sdk.l3_slp_gid.value);
  } else if (sdk.l2_slp_gid.is_valid) {
    out.sourceLp = static_cast<uint32_t>(sdk.l2_slp_gid.value);
  }
  if (sdk.l3_dlp_gid.is_valid) {
    out.destinationLp = static_cast<uint32_t>(sdk.l3_dlp_gid.value);
  } else if (sdk.l2_dlp_gid.is_valid) {
    out.destinationLp = static_cast<uint32_t>(sdk.l2_dlp_gid.value);
  }
  if (sdk.switch_gid.is_valid) {
    out.relayId = static_cast<uint16_t>(sdk.switch_gid.value);
  }
  if (sdk.transmit_time.is_valid) {
    out.timeStamp = static_cast<uint64_t>(sdk.transmit_time.value.count());
  }
  if (sdk.receive_time.is_valid) {
    out.receiveTime = static_cast<uint32_t>(sdk.receive_time.value.count());
  }

  out.nextHeaderKind = mapSdkNextHeaderKind(sdk.next_header_type);
  switch (out.nextHeaderKind) {
    case TajoPuntNextHeaderKind::Ethernet:
      out.nextHeader = 1;
      break;
    case TajoPuntNextHeaderKind::Ipv4:
      out.nextHeader = 4;
      break;
    case TajoPuntNextHeaderKind::Ipv6:
      out.nextHeader = 6;
      break;
    default:
      break;
  }
  if (sdk.tm_trap_reason.is_valid) {
    out.code = static_cast<uint8_t>(sdk.tm_trap_reason.value);
  } else if (sdk.lpts_reason.is_valid) {
    out.code = static_cast<uint8_t>(sdk.lpts_reason.value);
  } else if (sdk.mirror_code.is_valid) {
    out.code = static_cast<uint8_t>(sdk.mirror_code.value);
  } else if (sdk.redirect_code.is_valid) {
    out.code = static_cast<uint8_t>(sdk.redirect_code.value);
  }
  out.dropReason = out.code;
  setIngressPort(out);
}

using CreateParserFn =
    la_status (*)(la_device_family_e, silicon_one::la_header_parser*&);
using DestroyParserFn = la_status (*)(silicon_one::la_header_parser*);

CreateParserFn resolveCreateParserFn() {
  return &la_create_header_parser;
}

DestroyParserFn resolveDestroyParserFn() {
  return &la_destroy_header_parser;
}

std::vector<uint8_t> wireToSdkPuntBytes(const std::vector<uint8_t>& wireOrder) {
  auto sdkOrder = wireOrder;
  std::reverse(sdkOrder.begin(), sdkOrder.end());
  return sdkOrder;
}

std::optional<ParsedTajoPuntHeader> tryParseWithSdkFamily(
    const std::vector<uint8_t>& puntBytes,
    la_device_family_e deviceFamily,
    CreateParserFn createFn,
    DestroyParserFn destroyFn) {
  silicon_one::la_header_parser* parser = nullptr;
  auto status = createFn(deviceFamily, parser);
  if (status != LA_STATUS_SUCCESS || !parser) {
    return std::nullopt;
  }

  auto cleanup = folly::makeGuard([&]() { destroyFn(parser); });

  silicon_one::la_punt_header_t sdk{};
  if (parser->parse_punt_header(puntBytes, sdk) != LA_STATUS_SUCCESS) {
    return std::nullopt;
  }

  ParsedTajoPuntHeader out;
  out.parsedBySdk = true;
  out.consumedBytes = static_cast<uint32_t>(kTajoNplPuntHeaderBytes);
  populateFromSdkHeader(sdk, out);
  return out;
}

std::optional<ParsedTajoPuntHeader> tryParseWithSdk(folly::io::Cursor cursor) {
  auto* createFn = resolveCreateParserFn();
  auto* destroyFn = resolveDestroyParserFn();
  if (!createFn || !destroyFn) {
    return std::nullopt;
  }

  auto wireOrder = peekBytes(cursor, kTajoNplPuntHeaderBytes);
  if (wireOrder.size() != kTajoNplPuntHeaderBytes) {
    return std::nullopt;
  }

  const std::array<std::vector<uint8_t>, 2> puntCandidates = {
      wireToSdkPuntBytes(wireOrder),
      wireOrder,
  };

  constexpr la_device_family_e kFamilies[] = {
      la_device_family_e::GR2,
      la_device_family_e::G2LL,
      la_device_family_e::G204,
      la_device_family_e::GIBRALTAR,
      la_device_family_e::GRAPHENE,
  };
  for (const auto& puntBytes : puntCandidates) {
    for (auto family : kFamilies) {
      if (auto parsed =
              tryParseWithSdkFamily(puntBytes, family, createFn, destroyFn)) {
        return parsed;
      }
    }
  }
  return std::nullopt;
}
#endif

} // namespace

size_t tajoModExpectedOuterIpv6PayloadLen(
    size_t innerBytesOnWire,
    size_t truncateSize) {
  size_t innerSample = innerBytesOnWire;
  if (truncateSize > 0 &&
      truncateSize > kTajoModEneDummyHdrLen + kTajoNplPuntHeaderBytes) {
    const size_t maxInnerSample =
        truncateSize - kTajoModEneDummyHdrLen - kTajoNplPuntHeaderBytes;
    if (innerBytesOnWire > maxInnerSample) {
      innerSample = maxInnerSample;
    }
  }
  return kTajoModOuterUdpLen + kTajoNplPuntHeaderBytes + innerSample;
}

ParsedTajoPuntHeader parseTajoPuntHeader(folly::io::Cursor& cursor) {
#if TAJO_HAS_SDK_PUNT_HEADER_PARSER
  if (auto parsed = tryParseWithSdk(cursor)) {
    cursor.skip(parsed->consumedBytes);
    return *parsed;
  }
#endif

  if (auto parsed = tryParseWirePuntHeaderFromCursor(cursor)) {
    cursor.skip(parsed->consumedBytes);
    return *parsed;
  }

  ParsedTajoPuntHeader out;
  out.consumedBytes = static_cast<uint32_t>(kTajoNplPuntHeaderBytes);
  cursor.skip(kTajoNplPuntHeaderBytes);
  return out;
}

} // namespace facebook::fboss
