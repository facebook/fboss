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

#include "fboss/agent/hw/sai/api/SaiApiError.h"
#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/lib/TupleUtils.h"

#include <folly/logging/xlog.h>
#include <optional>

#include <tuple>

#include <boost/variant.hpp>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

template <typename AttrT>
const sai_attribute_t* saiAttr(const AttrT& attr) {
  static_assert(
      IsSaiAttribute<AttrT>::value,
      "cannot call saiAttrs() directly on a non SaiAttribute type");
  return attr.saiAttr();
}

template <typename AttrT>
const sai_attribute_t* saiAttr(const std::optional<AttrT>& opt) {
  if (opt) {
    return opt.value().saiAttr();
  }
  return nullptr;
}

template <typename... AttrTs>
std::vector<sai_attribute_t> saiAttrs(const std::tuple<AttrTs...>& tup) {
  std::vector<sai_attribute_t> ret;
  // tuple_size<std::tuple<Ts...>> may be too many, but certainly enough
  ret.reserve(std::tuple_size_v<std::tuple<AttrTs...>>);
  tupleForEach(
      [&ret](const auto& attr) {
        const sai_attribute_t* attrp = saiAttr(attr);
        if (attrp) {
          ret.push_back(*attrp);
        }
      },
      tup);
  return ret;
}

} // namespace facebook::fboss
