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
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace fmt {

template <typename SaiObjectTraits>
struct formatter<facebook::fboss::SaiObject<SaiObjectTraits>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(
      const facebook::fboss::SaiObject<SaiObjectTraits>& saiObject,
      FormatContext& ctx) const {
    return format_to(
        ctx.out(), "{}: {}", saiObject.adapterKey(), saiObject.attributes());
  }
};

template <typename SaiObjectTraits>
struct formatter<facebook::fboss::SaiObjectWithCounters<SaiObjectTraits>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(
      const facebook::fboss::SaiObjectWithCounters<SaiObjectTraits>& saiObject,
      FormatContext& ctx) const {
    return format_to(
        ctx.out(), "{}: {}", saiObject.adapterKey(), saiObject.attributes());
  }
};

} // namespace fmt
