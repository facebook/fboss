/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/prefix-list/CmdConfigProtocolBgpPolicyPrefixList.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <folly/IPAddress.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliAttrHandlers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliValueParsers.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {

// CLI keyword selecting the nested entry, and the attribute names, exactly as
// documented. Kept here so the valid-attribute sets and the handler tables
// stay in sync.
constexpr std::string_view kEntryKeyword = "entry";
constexpr std::string_view kBooleanOperator = "boolean-operator";
constexpr std::string_view kCompareOperator = "compare-operator";
constexpr std::string_view kDescription = "description";
constexpr std::string_view kIpVersion = "ip-version";
constexpr std::string_view kBasePrefix = "base-prefix";
constexpr std::string_view kCommunities = "communities";
constexpr std::string_view kMatchLogic = "match-logic";
constexpr std::string_view kMaxAllowedSubnetCount = "max-allowed-subnet-count";
constexpr std::string_view kPrefixLenRange = "prefix-len-range";
constexpr std::string_view kRegex = "regex";
constexpr std::string_view kValue = "value";

// boolean-operator values (routing_policy.BooleanOperator names).
constexpr std::string_view kBooleanOperatorAnd = "AND";
constexpr std::string_view kBooleanOperatorOr = "OR";
constexpr std::string_view kBooleanOperatorNot = "NOT";

// compare-operator values (routing_policy.ComparisonOperator names). RG
// (range) is only documented for the entry-level prefix-len-range.
constexpr std::string_view kCompareOperatorEq = "EQ";
constexpr std::string_view kCompareOperatorGe = "GE";
constexpr std::string_view kCompareOperatorLe = "LE";
constexpr std::string_view kCompareOperatorNe = "NE";
constexpr std::string_view kCompareOperatorGt = "GT";
constexpr std::string_view kCompareOperatorLt = "LT";
constexpr std::string_view kCompareOperatorRg = "RG";

// match-logic values (routing_policy.MatchValueLogicOperator names).
constexpr std::string_view kMatchLogicEqual = "EQUAL";
constexpr std::string_view kMatchLogicNotEqual = "NOT_EQUAL";

// ip-version values, stored as the numeric IP version in PrefixList.version
// (the documented thrift field for this command).
constexpr std::string_view kIpVersionV4 = "v4";
constexpr std::string_view kIpVersionV6 = "v6";
constexpr int32_t kIpVersionV4Value = 4;
constexpr int32_t kIpVersionV6Value = 6;

// prefix-len-range value bounds: a prefix length (v6 caps it at 128).
constexpr int32_t kPrefixLenMax = 128;

using BooleanOperator = bgp::routing_policy::BooleanOperator;
using ComparisonOperator = bgp::routing_policy::ComparisonOperator;
using CompareNumericValue = bgp::routing_policy::CompareNumericValue;
using MatchValueLogicOperator = bgp::routing_policy::MatchValueLogicOperator;
using PrefixList = bgp::routing_policy::PrefixList;
using PrefixListEntry = bgp::routing_policy::PrefixListEntry;
using bgpcli::AttrHandler;
using bgpcli::enumAttr;
using bgpcli::err;
using bgpcli::intAttr;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::Result;
using bgpcli::Tokens;

std::optional<ComparisonOperator> lookupComparisonOperator(
    const std::string& s) {
  if (s == kCompareOperatorEq) {
    return ComparisonOperator::EQ;
  }
  if (s == kCompareOperatorGe) {
    return ComparisonOperator::GE;
  }
  if (s == kCompareOperatorLe) {
    return ComparisonOperator::LE;
  }
  if (s == kCompareOperatorNe) {
    return ComparisonOperator::NE;
  }
  if (s == kCompareOperatorGt) {
    return ComparisonOperator::GT;
  }
  if (s == kCompareOperatorLt) {
    return ComparisonOperator::LT;
  }
  return std::nullopt;
}

// ---- list-level attribute handlers ----------------------------------------
const std::map<std::string, AttrHandler<PrefixList>, std::less<>>&
listAttrHandlers() {
  static const std::map<std::string, AttrHandler<PrefixList>, std::less<>>
      kHandlers = {
          {std::string(kBooleanOperator),
           enumAttr<PrefixList, BooleanOperator>(
               kBooleanOperator,
               fmt::format(
                   "{}|{}|{}",
                   kBooleanOperatorAnd,
                   kBooleanOperatorOr,
                   kBooleanOperatorNot),
               [](const std::string& s) -> std::optional<BooleanOperator> {
                 if (s == kBooleanOperatorAnd) {
                   return BooleanOperator::AND;
                 }
                 if (s == kBooleanOperatorOr) {
                   return BooleanOperator::OR;
                 }
                 if (s == kBooleanOperatorNot) {
                   return BooleanOperator::NOT;
                 }
                 return std::nullopt;
               },
               [](PrefixList& l, BooleanOperator op) {
                 l.boolean_operator() = op;
               })},
          {std::string(kCompareOperator),
           enumAttr<PrefixList, ComparisonOperator>(
               kCompareOperator,
               fmt::format(
                   "{}|{}|{}|{}|{}|{}",
                   kCompareOperatorEq,
                   kCompareOperatorGe,
                   kCompareOperatorLe,
                   kCompareOperatorNe,
                   kCompareOperatorGt,
                   kCompareOperatorLt),
               lookupComparisonOperator,
               [](PrefixList& l, ComparisonOperator op) {
                 l.compare_operator() = op;
               })},
          {std::string(kDescription),
           joinedStringAttr<PrefixList>(
               kDescription,
               [](PrefixList& l, const std::string& v) {
                 l.description() = v;
               })},
          {std::string(kIpVersion),
           enumAttr<PrefixList, int32_t>(
               kIpVersion,
               fmt::format("{}|{}", kIpVersionV4, kIpVersionV6),
               [](const std::string& s) -> std::optional<int32_t> {
                 if (s == kIpVersionV4) {
                   return kIpVersionV4Value;
                 }
                 if (s == kIpVersionV6) {
                   return kIpVersionV6Value;
                 }
                 return std::nullopt;
               },
               [](PrefixList& l, int32_t version) { l.version() = version; })},
      };
  return kHandlers;
}

// The attr factories keep their display name as a string_view, so the
// composed `prefix-len-range <sub-attr>` names need static storage — a
// fmt::format temporary at the factory call site would dangle (the same
// hazard enumAttr's valueDesc used to have).
const std::string& prefixLenRangeCompareOperatorName() {
  static const std::string kName =
      fmt::format("{} {}", kPrefixLenRange, kCompareOperator);
  return kName;
}

const std::string& prefixLenRangeValueName() {
  static const std::string kName =
      fmt::format("{} {}", kPrefixLenRange, kValue);
  return kName;
}

// ---- prefix-len-range sub-attribute handlers --------------------------------
// The thrift models a list of accepted length ranges (prefix_len_ranges); the
// documented CLI exposes a single range, so these target the one
// CompareNumericValue the entry handler maintains at prefix_len_ranges[0].
const std::map<std::string, AttrHandler<CompareNumericValue>, std::less<>>&
prefixLenRangeAttrHandlers() {
  static const std::
      map<std::string, AttrHandler<CompareNumericValue>, std::less<>>
          kHandlers = {
              {std::string(kCompareOperator),
               enumAttr<CompareNumericValue, ComparisonOperator>(
                   prefixLenRangeCompareOperatorName(),
                   fmt::format(
                       "{}|{}|{}|{}|{}|{}|{}",
                       kCompareOperatorEq,
                       kCompareOperatorGe,
                       kCompareOperatorLe,
                       kCompareOperatorNe,
                       kCompareOperatorGt,
                       kCompareOperatorLt,
                       kCompareOperatorRg),
                   [](const std::string& s)
                       -> std::optional<ComparisonOperator> {
                     if (s == kCompareOperatorRg) {
                       return ComparisonOperator::RG;
                     }
                     return lookupComparisonOperator(s);
                   },
                   [](CompareNumericValue& r, ComparisonOperator op) {
                     r.compare_operator() = op;
                   })},
              {std::string(kValue),
               intAttr<CompareNumericValue>(
                   prefixLenRangeValueName(),
                   fmt::format("0-{}", kPrefixLenMax),
                   0,
                   kPrefixLenMax,
                   [](CompareNumericValue& r, int32_t v) { r.value() = v; })},
          };
  return kHandlers;
}

const std::string& prefixLenRangeUsage() {
  static const std::string kUsage = fmt::format(
      "Error: {} requires <{}|{}> <value>",
      kPrefixLenRange,
      kCompareOperator,
      kValue);
  return kUsage;
}

// ---- entry-level attribute handlers ----------------------------------------
const std::map<std::string, AttrHandler<PrefixListEntry>, std::less<>>&
entryAttrHandlers() {
  static const std::map<std::string, AttrHandler<PrefixListEntry>, std::less<>>
      kHandlers = {
          {std::string(kBasePrefix),
           [](PrefixListEntry& e, const Tokens& values) -> Result {
             if (values.size() != 1) {
               return err(
                   fmt::format("Error: {} requires <prefix/len>", kBasePrefix));
             }
             // Require an explicit /len — folly fills in a default mask for a
             // bare address, so its absence must be checked separately. The
             // string is stored as typed, not normalized.
             if (values[0].find('/') == std::string::npos ||
                 folly::IPAddress::tryCreateNetwork(values[0]).hasError()) {
               return err(
                   fmt::format(
                       "Error: Invalid {} value '{}'; expected <prefix/len>",
                       kBasePrefix,
                       values[0]));
             }
             e.base_prefix() = values[0];
             return ok(
                 fmt::format(
                     "Successfully set {} to: {}", kBasePrefix, values[0]));
           }},
          {std::string(kCommunities),
           [](PrefixListEntry& e, const Tokens& values) -> Result {
             if (values.size() != 1) {
               return err(
                   fmt::format(
                       "Error: {} requires <community-string>", kCommunities));
             }
             // The accepted communities are a set; repeated invocations
             // accumulate members rather than replacing the set.
             const bool inserted =
                 e.communities().ensure().insert(values[0]).second;
             return ok(
                 inserted
                     ? fmt::format(
                           "Successfully added {} to {}",
                           values[0],
                           kCommunities)
                     : fmt::format(
                           "{} already contains {}", kCommunities, values[0]));
           }},
          {std::string(kDescription),
           joinedStringAttr<PrefixListEntry>(
               kDescription,
               [](PrefixListEntry& e, const std::string& v) {
                 e.description() = v;
               })},
          {std::string(kMatchLogic),
           enumAttr<PrefixListEntry, MatchValueLogicOperator>(
               kMatchLogic,
               fmt::format("{}|{}", kMatchLogicEqual, kMatchLogicNotEqual),
               [](const std::string& s)
                   -> std::optional<MatchValueLogicOperator> {
                 if (s == kMatchLogicEqual) {
                   return MatchValueLogicOperator::EQUAL;
                 }
                 if (s == kMatchLogicNotEqual) {
                   return MatchValueLogicOperator::NOT_EQUAL;
                 }
                 return std::nullopt;
               },
               [](PrefixListEntry& e, MatchValueLogicOperator op) {
                 e.match_logic() = op;
               })},
          {std::string(kMaxAllowedSubnetCount),
           intAttr<PrefixListEntry>(
               kMaxAllowedSubnetCount,
               "non-negative integer",
               0,
               std::numeric_limits<int32_t>::max(),
               [](PrefixListEntry& e, int32_t v) {
                 e.max_allowed_golden_prefix_subnet_count() = v;
               })},
          {std::string(kPrefixLenRange),
           [](PrefixListEntry& e, const Tokens& values) -> Result {
             if (values.empty()) {
               return err(prefixLenRangeUsage());
             }
             auto handler = prefixLenRangeAttrHandlers().find(values[0]);
             if (handler == prefixLenRangeAttrHandlers().end()) {
               return err(prefixLenRangeUsage());
             }
             auto& ranges = *e.prefix_len_ranges();
             const bool rangeCreated = ranges.empty();
             if (rangeCreated) {
               ranges.emplace_back();
             }
             auto result = handler->second(
                 ranges.front(), Tokens(values.begin() + 1, values.end()));
             // A rejected value must not leave a phantom range behind.
             if (!result.ok && rangeCreated) {
               ranges.pop_back();
             }
             return result;
           }},
          {std::string(kRegex),
           [](PrefixListEntry& e, const Tokens& values) -> Result {
             if (values.size() != 1) {
               return err(fmt::format("Error: {} requires <string>", kRegex));
             }
             // The thrift documents regex as an alternative to
             // prefix_len_ranges; the CLI stores what is given and leaves
             // precedence to the daemon.
             e.regex() = values[0];
             return ok(
                 fmt::format("Successfully set {} to: {}", kRegex, values[0]));
           }},
      };
  return kHandlers;
}

template <typename Handlers>
std::string validAttrList(const Handlers& handlers) {
  std::string out;
  for (const auto& [name, _] : handlers) {
    if (!out.empty()) {
      out += ", ";
    }
    out += name;
  }
  return out;
}

// Find the prefix-list keyed by name, creating it if absent. Setting an
// attribute on a not-yet-created list implicitly creates it, so command
// ordering stays forgiving; a bare `prefix-list <name>` creates one
// explicitly. PrefixList's only key field is `name`.
PrefixList& findOrCreateList(
    bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  auto& lists = *cfg.policies().ensure().prefix_lists();
  for (auto& list : lists) {
    if (*list.name() == name) {
      return list;
    }
  }
  lists.emplace_back();
  auto& list = lists.back();
  list.name() = name;
  return list;
}

// Find the entry keyed by seq_num within a list's prefixes[], creating it if
// absent. seq_num is the entry's identity.
PrefixListEntry& findOrCreateEntry(PrefixList& list, int32_t seqNum) {
  auto& entries = *list.prefixes();
  for (auto& entry : entries) {
    if (entry.seq_num().has_value() && *entry.seq_num() == seqNum) {
      return entry;
    }
  }
  entries.emplace_back();
  auto& entry = entries.back();
  entry.seq_num() = seqNum;
  return entry;
}

bool prefixListExists(
    const bgp::thrift::BgpConfig& cfg,
    const std::string& name) {
  if (!cfg.policies().has_value()) {
    return false;
  }
  for (const auto& list : *cfg.policies()->prefix_lists()) {
    if (*list.name() == name) {
      return true;
    }
  }
  return false;
}

bool prefixListEntryExists(const PrefixList& list, int32_t seqNum) {
  for (const auto& entry : *list.prefixes()) {
    if (entry.seq_num().has_value() && *entry.seq_num() == seqNum) {
      return true;
    }
  }
  return false;
}

} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
// Throwing std::invalid_argument is how the framework surfaces arg parse
// errors (same mechanism as BgpAsPathListConfig).
BgpPrefixListConfig::BgpPrefixListConfig(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: prefix-list <name> is required, optionally followed by "
        "`entry <seq-num>` and an <attribute> <value>");
  }
  if (v[0].empty()) {
    throw std::invalid_argument("Error: prefix-list name must not be empty");
  }
  listName_ = v[0];

  if (v.size() == 1) {
    return; // bare `prefix-list <name>`: create the list
  }

  // `entry <seq-num>` selects the nested entry; anything else is a list-level
  // attribute.
  size_t attrStart = 1;
  if (v[1] == kEntryKeyword) {
    if (v.size() < 3) {
      throw std::invalid_argument("Error: `entry` requires a <seq-num>");
    }
    auto seq = bgpcli::parseNonNegInt32(v[2]);
    if (!seq) {
      throw std::invalid_argument(
          fmt::format(
              "Error: entry <seq-num> must be a non-negative integer, got '{}'",
              v[2]));
    }
    hasEntry_ = true;
    seqNum_ = *seq;
    if (v.size() == 3) {
      return; // bare `prefix-list <name> entry <seq-num>`: create the entry
    }
    attrStart = 3;
  }

  attr_ = v[attrStart];
  values_.assign(v.begin() + attrStart + 1, v.end());

  const bool known = hasEntry_
      ? entryAttrHandlers().find(attr_) != entryAttrHandlers().end()
      : listAttrHandlers().find(attr_) != listAttrHandlers().end();
  if (!known) {
    throw std::invalid_argument(
        fmt::format(
            "Error: unknown prefix-list {} attribute '{}'. Valid "
            "attributes: {}",
            hasEntry_ ? "entry" : "list",
            attr_,
            hasEntry_ ? validAttrList(entryAttrHandlers())
                      : validAttrList(listAttrHandlers())));
  }
}

CmdConfigProtocolBgpPolicyPrefixListTraits::RetType
CmdConfigProtocolBgpPolicyPrefixList::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  const bool listCreated = !prefixListExists(cfg, args.listName());
  auto& list = findOrCreateList(cfg, args.listName());

  Result result;
  if (!args.hasEntry()) {
    result = args.attr().empty()
        ? ok(listCreated
                 ? fmt::format(
                       "Successfully created BGP prefix-list {}",
                       args.listName())
                 : fmt::format(
                       "BGP prefix-list {} already exists", args.listName()))
        // The attribute is guaranteed valid: BgpPrefixListConfig's constructor
        // rejects an unknown attribute before we get here.
        : listAttrHandlers().find(args.attr())->second(list, args.values());
  } else {
    const bool entryCreated = !prefixListEntryExists(list, args.seqNum());
    auto& entry = findOrCreateEntry(list, args.seqNum());
    result = args.attr().empty()
        ? ok(entryCreated
                 ? fmt::format(
                       "Successfully created BGP prefix-list {} entry {}",
                       args.listName(),
                       args.seqNum())
                 : fmt::format(
                       "BGP prefix-list {} entry {} already exists",
                       args.listName(),
                       args.seqNum()))
        : entryAttrHandlers().find(args.attr())->second(entry, args.values());
    // A rejected value must not leave a phantom entry behind. If the list was
    // freshly created too, the list-level rollback below removes it (and the
    // entry with it), so only roll back the entry when the list pre-existed.
    if (!result.ok && entryCreated && !listCreated) {
      list.prefixes()->pop_back();
    }
  }

  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += args.hasEntry()
          ? fmt::format(
                " for prefix-list {} entry {}", args.listName(), args.seqNum())
          : fmt::format(" for prefix-list {}", args.listName());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else if (listCreated) {
    // Drop the phantom list (and any entry created under it this invocation)
    // so a rejected value is not visible to later lookups in the same process.
    cfg.policies()->prefix_lists()->pop_back();
  }
  return result.message;
}

void CmdConfigProtocolBgpPolicyPrefixList::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigProtocolBgpPolicyPrefixList,
    CmdConfigProtocolBgpPolicyPrefixListTraits>::run();

} // namespace facebook::fboss
