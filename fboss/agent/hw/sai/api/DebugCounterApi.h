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

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class DebugCounterApi;

struct SaiDebugCounterTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_DEBUG_COUNTER;
  struct Attributes {
    using EnumType = sai_debug_counter_attr_t;
    using Index =
        SaiAttribute<EnumType, SAI_DEBUG_COUNTER_ATTR_INDEX, sai_uint32_t>;
    using Type = SaiAttribute<
        EnumType,
        SAI_DEBUG_COUNTER_ATTR_TYPE,
        sai_debug_counter_type_t>;
    using BindMethod = SaiAttribute<
        EnumType,
        SAI_DEBUG_COUNTER_ATTR_BIND_METHOD,
        sai_debug_counter_bind_method_t>;
    using InDropReasons = SaiAttribute<
        EnumType,
        SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST,
        std::vector<int32_t>>;
    using OutDropReasons = SaiAttribute<
        EnumType,
        SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST,
        std::vector<int32_t>>;
  };
  using AdapterKey = DebugCounterSaiId;
  using CreateAttributes = std::tuple<
      Attributes::Type,
      Attributes::BindMethod,
      std::optional<Attributes::InDropReasons>,
      std::optional<Attributes::OutDropReasons>>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(DebugCounter, Index)
SAI_ATTRIBUTE_NAME(DebugCounter, Type)
SAI_ATTRIBUTE_NAME(DebugCounter, BindMethod)
SAI_ATTRIBUTE_NAME(DebugCounter, InDropReasons)
SAI_ATTRIBUTE_NAME(DebugCounter, OutDropReasons)
} // namespace facebook::fboss
