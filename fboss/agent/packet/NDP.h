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

#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/types.h"

#include <folly/io/Cursor.h>

namespace facebook::fboss {

/**
 * Constants for NDP option type fields (RFC 4861 (sec 4.6))
 *
 * We're using a nested enum inside a class to get "enum class" like syntax,
 * but without the strict type checking, so we can still easily convert to and
 * from uint8_t.
 */
struct NDPOptionType {
  enum Values : uint8_t {
    SRC_LL_ADDRESS = 1,
    TARGET_LL_ADDRESS = 2,
    PREFIX_INFO = 3,
    REDIRECTED_HEADER = 4,
    MTU = 5,
  };
};

/**
 * Length constants for fixed-length NDP option types.
 */
struct NDPOptionLength {
  enum Values : uint8_t {
    SRC_LL_ADDRESS_IEEE802 = 1,
    TARGET_LL_ADDRESS_IEEE802 = 1,
    PREFIX_INFO = 4,
    // REDIRECTED_HEADER is variable length
    MTU = 1,
  };
};

struct NeighborAdvertisementFlags {
  enum Values : uint32_t {
    ROUTER = 1UL << 31,
    SOLICITED = 1UL << 30,
    OVERRIDE = 1UL << 29,
  };
};

class NDPOptionHdr {
 public:
  uint8_t type() const {
    return type_;
  }
  uint8_t length() const {
    return length_;
  }
  uint16_t payloadLength() const {
    return sizeof(uint64_t) * length() - hdrLength();
  }
  static constexpr uint8_t hdrLength() {
    return 2;
  }

  explicit NDPOptionHdr(folly::io::Cursor& cursor);

 private:
  uint8_t type_;
  uint8_t length_;
};

class NDPOptions {
 public:
  std::optional<uint32_t> mtu{std::nullopt};
  std::optional<folly::MacAddress> sourceLinkLayerAddress{std::nullopt};
  std::optional<folly::MacAddress> targetLinkLayerAddress{std::nullopt};

  NDPOptions() {}
  explicit NDPOptions(folly::io::Cursor& cursor);

  void tryParse(folly::io::Cursor& cursor);

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  size_t computeTotalLength() const;

 private:
  void getMtu(const NDPOptionHdr& ndpHdr, folly::io::Cursor& cursor);
  void getSourceLinkLayerAddress(
      const NDPOptionHdr& ndpHdr,
      folly::io::Cursor& cursor);
  void getTargetLinkLayerAddress(
      const NDPOptionHdr& ndpHdr,
      folly::io::Cursor& cursor);
  void skipOption(const NDPOptionHdr& ndpHdr, folly::io::Cursor& cursor);
};

} // namespace facebook::fboss
