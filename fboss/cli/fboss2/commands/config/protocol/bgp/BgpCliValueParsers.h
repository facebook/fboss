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

#include <folly/Conv.h>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <utility>

/**
 * Value parsing helpers shared by the BGP config dispatchers
 * (`config protocol bgp global` / `config protocol bgp neighbor`).
 *
 * All parsers return std::nullopt on invalid input instead of throwing so a
 * handler can reject the value with a user-facing message and leave the
 * session unpersisted.
 */
namespace facebook::fboss::bgpcli {

// Outcome of an attribute handler: on failure the message is returned to the
// user and the session is NOT persisted, so a rejected value never lands on
// disk.
struct Result {
  bool ok;
  std::string message;
};

inline Result ok(std::string message) {
  return Result{true, std::move(message)};
}

inline Result err(std::string message) {
  return Result{false, std::move(message)};
}

inline std::optional<bool> parseBool(const std::string& value) {
  if (value == "true" || value == "1" || value == "yes") {
    return true;
  }
  if (value == "false" || value == "0" || value == "no") {
    return false;
  }
  return std::nullopt;
}

template <typename T>
std::optional<T> parseInt(const std::string& value) {
  try {
    return folly::to<T>(value);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

// Parse a non-negative value that must fit in int32 (used for second-valued
// timers and min-routes).
inline std::optional<int32_t> parseNonNegInt32(const std::string& value) {
  auto parsed = parseInt<int64_t>(value);
  if (!parsed || *parsed < 0 || *parsed > std::numeric_limits<int32_t>::max()) {
    return std::nullopt;
  }
  return static_cast<int32_t>(*parsed);
}

// Parse a non-negative int64 (used for route limits).
inline std::optional<int64_t> parseNonNegInt64(const std::string& value) {
  auto parsed = parseInt<int64_t>(value);
  if (!parsed || *parsed < 0) {
    return std::nullopt;
  }
  return parsed;
}

// Parse a 4-byte ASN (RFC 6793): an unsigned value in [0, 2^32-1]. The thrift
// fields are i64, so an unchecked uint64 parse would let an out-of-range ASN
// wrap or exceed the protocol range and be persisted.
inline std::optional<int64_t> parseAsn4Byte(const std::string& value) {
  auto parsed = parseInt<uint64_t>(value);
  if (!parsed || *parsed > std::numeric_limits<uint32_t>::max()) {
    return std::nullopt;
  }
  return static_cast<int64_t>(*parsed);
}

} // namespace facebook::fboss::bgpcli
