// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace facebook::fboss {

// Helpers shared by the per ASIC drop reason reporting paths. Vendors expose
// active drop reasons differently -- Chenab as per pipeline stage bitmaps,
// Broadcom XGS as flat lists of enum values -- but both end up turning
// stringified SDK enumerators into a single human readable log line. Sharing
// that here keeps the log format identical across ASICs.

// Strip `prefix` and `suffix` from a stringified SDK enumerator, along with any
// " = <value>" tail left behind by stringifying an enum entry. Returns empty
// for a null input.
std::string_view extractDropReasonName(
    const char* raw,
    std::string_view prefix,
    std::string_view suffix);

// Join names with ", ", capping the result at a fixed length and appending
// "<truncated>" when the cap is reached. Empty names are skipped.
//
// The string_view overload is for callers whose names already point into the
// stringified SDK enumerator tables, which outlive the call: they can join
// without materializing a std::string per name. Callers that have to
// synthesize a name, as the Broadcom path does for values its build does not
// know, own strings and use the other.
std::string joinDropReasonNames(const std::vector<std::string>& names);
std::string joinDropReasonNames(const std::vector<std::string_view>& names);

// `enumNames` is indexed by bit position. Bits past its end, or past the width
// of the bitmap, are ignored.
std::string decodeDropBitmap(
    int64_t bitmap,
    std::span<const char* const> enumNames,
    std::string_view prefix,
    std::string_view suffix);

} // namespace facebook::fboss
