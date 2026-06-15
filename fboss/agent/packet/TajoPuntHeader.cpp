// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/TajoPuntHeader.h"

#include <dlfcn.h>

#include "folly/ScopeGuard.h"

#if __has_include(                             \
    "api/header_parser/la_header_parser.h") && \
    __has_include("api/header_parser/la_punt_header.h")
#include "api/header_parser/la_header_parser.h"
#include "api/header_parser/la_punt_header.h"
#define TAJO_HAS_SDK_PUNT_HEADER_PARSER 1
#else
#define TAJO_HAS_SDK_PUNT_HEADER_PARSER 0
#endif

namespace facebook::fboss {

namespace {

struct TajoLegacyPuntHeader {
  uint8_t version{};
  uint8_t flags{};
  uint16_t sequence{};
  uint16_t dropReason{};
  uint8_t dropFlags{};
  uint8_t reasonExt{};
  uint16_t subType{};
  uint16_t dropCategory{};
  uint16_t reserved1{};
  uint16_t reserved2{};
  uint16_t observationDomain{};
  uint16_t flowId{};
  uint16_t ingressPort{};
  uint16_t ingressInfo{};
  uint16_t egressPort{};
  uint16_t egressInfo{};
} __attribute__((packed));
static_assert(sizeof(TajoLegacyPuntHeader) == 28);

#if TAJO_HAS_SDK_PUNT_HEADER_PARSER
using CreateParserFn =
    la_status (*)(la_device_family_e, silicon_one::la_header_parser*&);
using DestroyParserFn = la_status (*)(silicon_one::la_header_parser*);

std::optional<ParsedTajoPuntHeader> tryParseWithSdk(folly::io::Cursor& cursor) {
  void* self = dlopen(nullptr, RTLD_LAZY);
  if (!self) {
    return std::nullopt;
  }
  auto* createFn =
      reinterpret_cast<CreateParserFn>(dlsym(self, "la_create_header_parser"));
  auto* destroyFn = reinterpret_cast<DestroyParserFn>(
      dlsym(self, "la_destroy_header_parser"));
  if (!createFn || !destroyFn) {
    dlclose(self);
    return std::nullopt;
  }

  silicon_one::la_header_parser* parser = nullptr;
  auto status = createFn(la_device_family_e::GIBRALTAR, parser);
  if (status != LA_STATUS_SUCCESS || !parser) {
    dlclose(self);
    return std::nullopt;
  }

  auto cleanup = folly::makeGuard([&]() {
    destroyFn(parser);
    dlclose(self);
  });

  for (auto headerBytes : {size_t(40), size_t(28)}) {
    folly::io::Cursor probe(cursor);
    std::vector<la_uint8_t> raw;
    raw.reserve(headerBytes);
    bool enough = true;
    for (size_t i = 0; i < headerBytes; ++i) {
      if (!probe.canAdvance(1)) {
        enough = false;
        break;
      }
      raw.push_back(probe.read<uint8_t>());
    }
    if (!enough) {
      continue;
    }

    silicon_one::la_punt_header_t sdk{};
    if (parser->parse_punt_header(raw, sdk) != LA_STATUS_SUCCESS) {
      continue;
    }

    ParsedTajoPuntHeader out;
    out.parsedBySdk = true;
    out.consumedBytes = static_cast<uint32_t>(headerBytes);
    if (sdk.l3_slp_gid.is_valid) {
      out.ingressPort = static_cast<uint16_t>(sdk.l3_slp_gid.value);
    } else if (sdk.l2_slp_gid.is_valid) {
      out.ingressPort = static_cast<uint16_t>(sdk.l2_slp_gid.value);
    } else if (sdk.ssp_gid.is_valid) {
      out.ingressPort = static_cast<uint16_t>(sdk.ssp_gid.value);
    }
    if (sdk.tm_trap_reason.is_valid) {
      out.dropReason = static_cast<uint16_t>(sdk.tm_trap_reason.value);
    } else if (sdk.lpts_reason.is_valid) {
      out.dropReason = static_cast<uint16_t>(sdk.lpts_reason.value);
    }
    return out;
  }

  return std::nullopt;
}
#endif

ParsedTajoPuntHeader parseLegacy(folly::io::Cursor& cursor) {
  ParsedTajoPuntHeader out;
  TajoLegacyPuntHeader hdr;

  hdr.version = cursor.read<uint8_t>();
  hdr.flags = cursor.read<uint8_t>();
  hdr.sequence = cursor.readBE<uint16_t>();
  hdr.dropReason = cursor.readBE<uint16_t>();
  hdr.dropFlags = cursor.read<uint8_t>();
  hdr.reasonExt = cursor.read<uint8_t>();
  hdr.subType = cursor.readBE<uint16_t>();
  hdr.dropCategory = cursor.readBE<uint16_t>();
  hdr.reserved1 = cursor.readBE<uint16_t>();
  hdr.reserved2 = cursor.readBE<uint16_t>();
  hdr.observationDomain = cursor.readBE<uint16_t>();
  hdr.flowId = cursor.readBE<uint16_t>();
  hdr.ingressPort = cursor.readBE<uint16_t>();
  hdr.ingressInfo = cursor.readBE<uint16_t>();
  hdr.egressPort = cursor.readBE<uint16_t>();
  hdr.egressInfo = cursor.readBE<uint16_t>();

  out.ingressPort = hdr.ingressPort;
  out.dropReason = hdr.dropReason;
  out.version = hdr.version;
  out.subType = hdr.subType;
  out.reasonExt = hdr.reasonExt;
  out.reserved1 = hdr.reserved1;
  out.reserved2 = hdr.reserved2;
  out.consumedBytes = sizeof(TajoLegacyPuntHeader);
  return out;
}

} // namespace

ParsedTajoPuntHeader parseTajoPuntHeader(folly::io::Cursor& cursor) {
#if TAJO_HAS_SDK_PUNT_HEADER_PARSER
  if (auto parsed = tryParseWithSdk(cursor)) {
    cursor.skip(parsed->consumedBytes);
    return *parsed;
  }
#endif
  return parseLegacy(cursor);
}

} // namespace facebook::fboss
