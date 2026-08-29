// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/DropReasonUtils.h"

#include <algorithm>
#include <climits>
#include <span>

#include <folly/logging/xlog.h>

namespace facebook::fboss {

namespace {
constexpr size_t kMaxReasonStringLen = 256;
constexpr std::string_view kSeparator = ", ";
constexpr std::string_view kTruncatedSuffix = "<truncated>";
} // namespace

std::string_view extractDropReasonName(
    const char* raw,
    std::string_view prefix,
    std::string_view suffix) {
  if (!raw) {
    return {};
  }
  std::string_view sv(raw);
  if (sv.starts_with(prefix)) {
    sv.remove_prefix(prefix.size());
  }
  auto eqPos = sv.find(" = ");
  if (eqPos != std::string_view::npos) {
    sv = sv.substr(0, eqPos);
  }
  if (sv.ends_with(suffix)) {
    sv.remove_suffix(suffix.size());
  }
  return sv;
}

namespace {
template <typename Names>
std::string joinNames(const Names& names) {
  std::string result;
  result.reserve(
      kMaxReasonStringLen + kSeparator.size() + kTruncatedSuffix.size());
  for (const auto& name : names) {
    if (name.empty()) {
      continue;
    }
    auto sepLen = result.empty() ? 0 : kSeparator.size();
    if (result.size() + sepLen + name.size() > kMaxReasonStringLen) {
      if (!result.empty()) {
        result += kSeparator;
      }
      result += kTruncatedSuffix;
      return result;
    }
    if (!result.empty()) {
      result += kSeparator;
    }
    result += name;
  }
  return result;
}
} // namespace

std::string joinDropReasonNames(const std::vector<std::string>& names) {
  return joinNames(names);
}

std::string joinDropReasonNames(const std::vector<std::string_view>& names) {
  return joinNames(names);
}

std::string decodeDropBitmap(
    int64_t bitmap,
    std::span<const char* const> enumNames,
    std::string_view prefix,
    std::string_view suffix) {
  if (enumNames.empty()) {
    // Rate-limited: an empty table is a persistent programming error and
    // decodeDropBitmap is called once per bitmap field per collection cycle,
    // so cap logging to avoid flooding.
    XLOG_EVERY_MS(ERR, 60000)
        << "decodeDropBitmap called with an empty enum table";
    return {};
  }
  // Views into enumNames, which is a static table of stringified enumerators
  // and so outlives this call.
  std::vector<std::string_view> names;
  auto bits = static_cast<uint64_t>(bitmap);
  constexpr size_t kBitmapWidth = sizeof(uint64_t) * CHAR_BIT;
  size_t limit = std::min(enumNames.size(), kBitmapWidth);
  for (size_t i = 0; i < limit; i++) {
    if (!(bits & (1ULL << i))) {
      continue;
    }
    auto name = extractDropReasonName(enumNames[i], prefix, suffix);
    if (!name.empty()) {
      names.emplace_back(name);
    }
  }
  return joinDropReasonNames(names);
}

} // namespace facebook::fboss
