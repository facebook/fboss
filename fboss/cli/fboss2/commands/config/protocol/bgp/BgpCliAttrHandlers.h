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

#include <fmt/core.h>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliValueParsers.h"
#include "fmt/format.h"

/**
 * Generic, target-typed per-attribute handler factories shared by the BGP
 * `config protocol bgp policy ...` dispatchers.
 *
 * Unlike the per-command factories in the neighbor/peer-group dispatchers
 * (which are hard-wired to a single thrift struct), these are templated on the
 * target type T so the same parsing/validation logic can mutate any policy
 * object (AsPathList, AsPathListEntry, and — as later object types land —
 * CommunityList, PrefixList, BgpPolicyTerm, ...). A handler validates its
 * tokens and either mutates T or returns an error the caller surfaces without
 * persisting.
 */
namespace facebook::fboss::bgpcli {

using Tokens = std::vector<std::string>;

// A per-attribute handler mutating a typed thrift target T in place.
template <typename T>
using AttrHandler = std::function<Result(T&, const Tokens&)>;

// A single-token string value (names, regex patterns, policy names).
template <typename T>
AttrHandler<T> stringAttr(
    std::string_view name,
    std::string_view valueName,
    std::function<void(T&, const std::string&)> set) {
  return [name, valueName, set = std::move(set)](
             T& target, const Tokens& values) -> Result {
    if (values.size() != 1) {
      return err(fmt::format("Error: {} requires <{}>", name, valueName));
    }
    set(target, values[0]);
    return ok(fmt::format("Successfully set {} to: {}", name, values[0]));
  };
}

// A free-text string value that may span multiple CLI tokens (descriptions);
// the tokens are re-joined with single spaces.
template <typename T>
AttrHandler<T> joinedStringAttr(
    std::string_view name,
    std::function<void(T&, const std::string&)> set) {
  return
      [name, set = std::move(set)](T& target, const Tokens& values) -> Result {
        if (values.empty()) {
          return err(fmt::format("Error: {} requires <string>", name));
        }
        std::string joined = values[0];
        for (size_t i = 1; i < values.size(); ++i) {
          joined += " " + values[i];
        }
        set(target, joined);
        return ok(fmt::format("Successfully set {} to: {}", name, joined));
      };
}

// A value drawn from a fixed set of names, mapped to an enum by `lookup`
// (returns std::nullopt for an unrecognized name). `valueDesc` documents the
// accepted names in both the usage and the rejection message.
template <typename T, typename Enum>
AttrHandler<T> enumAttr(
    std::string_view name,
    std::string_view valueDesc,
    std::function<std::optional<Enum>(const std::string&)> lookup,
    std::function<void(T&, Enum)> set) {
  return [name, valueDesc, lookup = std::move(lookup), set = std::move(set)](
             T& target, const Tokens& values) -> Result {
    if (values.size() != 1) {
      return err(fmt::format("Error: {} requires <{}>", name, valueDesc));
    }
    auto parsed = lookup(values[0]);
    if (!parsed) {
      return err(
          fmt::format(
              "Error: Invalid {} value '{}'; expected {}",
              name,
              values[0],
              valueDesc));
    }
    set(target, *parsed);
    return ok(fmt::format("Successfully set {} to: {}", name, values[0]));
  };
}

} // namespace facebook::fboss::bgpcli
