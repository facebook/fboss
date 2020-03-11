/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 * *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/api/LoggingUtil.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace fmt {

// Formatting for std::optional<SaiAttribute>
template <typename T>
struct formatter<std::optional<T>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const std::optional<T>& opt, FormatContext& ctx) {
    static_assert(
        facebook::fboss::IsSaiAttribute<T>::value,
        "format(std::optional) only valid for SaiAttributes");
    if (opt) {
      return format_to(ctx.out(), "{}", opt.value());
    } else {
      return format_to(ctx.out(), "omitted");
    }
  }
};

// Formatting for SaiObject
template <typename SaiObjectTraits>
struct formatter<facebook::fboss::SaiObject<SaiObjectTraits>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(
      const facebook::fboss::SaiObject<SaiObjectTraits>& saiObject,
      FormatContext& ctx) {
    return format_to(
        ctx.out(), "{}: {}", saiObject.adapterKey(), saiObject.attributes());
  }
};

} // namespace fmt
