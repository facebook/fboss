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

#include <cstdint>

#include <folly/io/Cursor.h>

namespace facebook::fboss::psamp {

// IPFIX message header per RFC 5101 Section 3.1.
constexpr uint16_t IPFIX_VERSION = 10;

struct IpfixHeader {
  uint16_t version{IPFIX_VERSION};
  uint16_t length{};
  uint32_t exportTime{};
  uint32_t sequenceNumber{};
  uint32_t observationDomainId{};

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static IpfixHeader deserialize(folly::io::Cursor& cursor);
};

} // namespace facebook::fboss::psamp
