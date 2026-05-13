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

#include <fmt/format.h>
#include <algorithm>
#include <cctype>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

// Single source of truth for the lldp-expected-* attribute names and their
// LLDPTag mapping, shared by `config interface` and `delete interface`.
inline const std::vector<std::pair<std::string, cfg::LLDPTag>>&
lldpAttrToTag() {
  static const std::vector<std::pair<std::string, cfg::LLDPTag>> kAttrToTag = {
      {"lldp-expected-value", cfg::LLDPTag::PORT},
      {"lldp-expected-chassis", cfg::LLDPTag::CHASSIS},
      {"lldp-expected-ttl", cfg::LLDPTag::TTL},
      {"lldp-expected-port-desc", cfg::LLDPTag::PORT_DESC},
      {"lldp-expected-system-name", cfg::LLDPTag::SYSTEM_NAME},
      {"lldp-expected-system-desc", cfg::LLDPTag::SYSTEM_DESC},
  };
  return kAttrToTag;
}

// Maps an lldp-expected-* attribute name to its LLDPTag enum value.
// Returns nullopt if attr is not a recognised lldp-expected-* attribute.
inline std::optional<cfg::LLDPTag> lldpTagForAttr(const std::string& attr) {
  for (const auto& [name, tag] : lldpAttrToTag()) {
    if (name == attr) {
      return tag;
    }
  }
  return std::nullopt;
}

// lldp-expected-* attribute names, in declaration order.
inline std::vector<std::string> lldpAttrNames() {
  std::vector<std::string> names;
  names.reserve(lldpAttrToTag().size());
  for (const auto& [name, tag] : lldpAttrToTag()) {
    names.push_back(name);
  }
  return names;
}

/*
 * Common base for CLI argument types that parse a token list of the form
 *   <port-list> [<attr> [<value>] ...]
 * into an InterfaceList plus (attribute, value) pairs. Tokens before the
 * first known attribute name are interface/port names; valueful attributes
 * consume the following token, valueless attributes do not.
 */
class InterfaceAttrArgsBase : public utils::BaseObjectArgType<std::string> {
 public:
  /* Get the resolved interfaces. */
  const utils::InterfaceList& getInterfaces() const {
    return interfaces_;
  }

  /* Implicit conversion to InterfaceList so that child commands can accept
   * const InterfaceList& without knowing about the concrete arg type. */
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ operator const utils::InterfaceList&() const {
    return interfaces_;
  }

  /* Get the (attribute, value) pairs. Value is empty for valueless attrs. */
  const std::vector<std::pair<std::string, std::string>>& getAttributes()
      const {
    return attributes_;
  }

  /* Check if any attributes were provided. */
  bool hasAttributes() const {
    return !attributes_.empty();
  }

 protected:
  InterfaceAttrArgsBase() : interfaces_(std::vector<std::string>{}) {}

  /*
   * Splits v into port names and attribute/value pairs (stored into
   * attributes_, names lowercased). Returns the unresolved port names so the
   * caller can run extra validation before resolving them into interfaces_.
   * attrKind ("attribute" / "delete attribute") and validAttrs are used to
   * compose the unknown-attribute error message.
   */
  std::vector<std::string> parseTokens(
      const std::vector<std::string>& v,
      const std::function<bool(const std::string&)>& isKnownAttr,
      const std::function<bool(const std::string&)>& isValuelessAttr,
      const std::string& attrKind,
      const std::string& validAttrs) {
    if (v.empty()) {
      throw std::invalid_argument("No interface name provided");
    }

    // Tokens before the first known attribute name are interface/port names.
    size_t attrStart = v.size();
    for (size_t i = 0; i < v.size(); ++i) {
      if (isKnownAttr(v[i])) {
        attrStart = i;
        break;
      }
    }

    // Must have at least one port name.
    if (attrStart == 0) {
      throw std::invalid_argument(
          fmt::format(
              "No interface name provided. First token '{}' is an attribute name.",
              v[0]));
    }

    std::vector<std::string> portNames(v.begin(), v.begin() + attrStart);

    // Parse attribute-value pairs (valueless attributes consume no value
    // token).
    for (size_t i = attrStart; i < v.size();) {
      const std::string& attr = v[i];
      if (!isKnownAttr(attr)) {
        throw std::invalid_argument(
            fmt::format(
                "Unknown {} '{}'. Valid attributes are: {}",
                attrKind,
                attr,
                validAttrs));
      }

      // Normalize attribute name to lowercase
      std::string attrLower = attr;
      std::transform(
          attrLower.begin(), attrLower.end(), attrLower.begin(), ::tolower);

      if (isValuelessAttr(attrLower)) {
        attributes_.emplace_back(std::move(attrLower), "");
        ++i;
        continue;
      }

      if (i + 1 >= v.size()) {
        throw std::invalid_argument(
            fmt::format("Missing value for attribute '{}'", attr));
      }

      const std::string& value = v[i + 1];

      // Check if "value" is actually another attribute name (user forgot
      // value)
      if (isKnownAttr(value)) {
        throw std::invalid_argument(
            fmt::format(
                "Missing value for attribute '{}'. Got another attribute '{}' instead.",
                attr,
                value));
      }

      attributes_.emplace_back(std::move(attrLower), value);
      i += 2;
    }

    return portNames;
  }

  utils::InterfaceList interfaces_;
  std::vector<std::pair<std::string, std::string>> attributes_;
};

} // namespace facebook::fboss
