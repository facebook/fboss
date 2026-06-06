// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>

#include <folly/io/Cursor.h>

namespace facebook::fboss {

struct ParsedTajoPuntHeader {
  uint16_t ingressPort{0};
  uint16_t dropReason{0};
  uint8_t version{0};
  uint16_t subType{0};
  uint8_t reasonExt{0};
  uint16_t reserved1{0};
  uint16_t reserved2{0};
  uint32_t consumedBytes{0};
  bool parsedBySdk{false};
};

// Decode Tajo punt header from cursor and advance cursor by the punt header
// byte length consumed.
//
// Preferred path is SDK parser model (la_punt_header_t semantics). If parser
// symbols are unavailable at runtime, decode falls back to legacy 28-byte wire
// format used by older captures.
ParsedTajoPuntHeader parseTajoPuntHeader(folly::io::Cursor& cursor);

} // namespace facebook::fboss
