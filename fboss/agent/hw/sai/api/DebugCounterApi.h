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
  using SaiApiT = DebugCounterApi;
  struct Attributes {
    using EnumType = sai_debug_counter_attr_t;
    using Index =
        SaiAttribute<EnumType, SAI_DEBUG_COUNTER_ATTR_INDEX, sai_uint32_t>;
    using Type =
        SaiAttribute<EnumType, SAI_DEBUG_COUNTER_ATTR_TYPE, sai_int32_t>;
    using BindMethod =
        SaiAttribute<EnumType, SAI_DEBUG_COUNTER_ATTR_BIND_METHOD, sai_int32_t>;
    using InDropReasons = SaiAttribute<
        EnumType,
        SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST,
        std::vector<int32_t>>;
  };
  using AdapterKey = DebugCounterSaiId;
  using CreateAttributes = std::tuple<
      Attributes::Type,
      Attributes::BindMethod,
      std::optional<Attributes::InDropReasons>>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(DebugCounter, Index)
SAI_ATTRIBUTE_NAME(DebugCounter, Type)
SAI_ATTRIBUTE_NAME(DebugCounter, BindMethod)
SAI_ATTRIBUTE_NAME(DebugCounter, InDropReasons)

class DebugCounterApi : public SaiApi<DebugCounterApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_DEBUG_COUNTER;
  DebugCounterApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for debug counter api");
  }
  sai_status_t _create(
      DebugCounterSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_debug_counter(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(DebugCounterSaiId id) {
    return api_->remove_debug_counter(id);
  }

  sai_status_t _getAttribute(DebugCounterSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_debug_counter_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(
      DebugCounterSaiId id,
      const sai_attribute_t* attr) {
    return api_->set_debug_counter_attribute(id, attr);
  }

  sai_debug_counter_api_t* api_;
  friend class SaiApi<DebugCounterApi>;
};
} // namespace facebook::fboss
