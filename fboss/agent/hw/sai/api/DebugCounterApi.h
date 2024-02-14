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

namespace detail {
template <sai_debug_counter_type_t>
struct DebugCounterAttributeTypes;

using _InDropReasons = SaiAttribute<
    sai_debug_counter_attr_t,
    SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST,
    std::vector<int32_t>>;
template <>
struct DebugCounterAttributeTypes<SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS> {
  using DropReasons = _InDropReasons;
};
template <>
struct DebugCounterAttributeTypes<
    SAI_DEBUG_COUNTER_TYPE_SWITCH_IN_DROP_REASONS> {
  using DropReasons = _InDropReasons;
};

using _OutDropReasons = SaiAttribute<
    sai_debug_counter_attr_t,
    SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST,
    std::vector<int32_t>>;
template <>
struct DebugCounterAttributeTypes<
    SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS> {
  using DropReasons = _OutDropReasons;
};
template <>
struct DebugCounterAttributeTypes<
    SAI_DEBUG_COUNTER_TYPE_SWITCH_OUT_DROP_REASONS> {
  using DropReasons = _OutDropReasons;
};
std::optional<sai_int32_t> trapDrops();
} // namespace detail

template <sai_debug_counter_type_t type>
struct SaiDebugCounterTraitsT {
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
    using DropReasons =
        typename detail::DebugCounterAttributeTypes<type>::DropReasons;
  };
  using AdapterKey = DebugCounterSaiId;
  using CreateAttributes = std::tuple<
      typename Attributes::Type,
      typename Attributes::BindMethod,
      std::optional<typename Attributes::DropReasons>>;
  using AdapterHostKey = CreateAttributes;
  static std::optional<sai_int32_t> trapDrops() {
    return detail::trapDrops();
  }
  using ConditionAttributes = std::tuple<typename Attributes::Type>;
  inline const static ConditionAttributes kConditionAttributes{type};
};

using SaiInPortDebugCounterTraits =
    SaiDebugCounterTraitsT<SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS>;
using SaiOutPortDebugCounterTraits =
    SaiDebugCounterTraitsT<SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS>;

template <>
struct SaiObjectHasConditionalAttributes<SaiInPortDebugCounterTraits>
    : public std::true_type {};
template <>
struct SaiObjectHasConditionalAttributes<SaiOutPortDebugCounterTraits>
    : public std::true_type {};

using SaiDebugCounterTraits = ConditionObjectTraits<
    SaiInPortDebugCounterTraits,
    SaiOutPortDebugCounterTraits>;

SAI_ATTRIBUTE_NAME(InPortDebugCounter, Index)
SAI_ATTRIBUTE_NAME(InPortDebugCounter, Type)
SAI_ATTRIBUTE_NAME(InPortDebugCounter, BindMethod)
SAI_ATTRIBUTE_NAME(InPortDebugCounter, DropReasons)
SAI_ATTRIBUTE_NAME(OutPortDebugCounter, DropReasons)

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
      sai_attribute_t* attr_list) const {
    return api_->create_debug_counter(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(DebugCounterSaiId id) const {
    return api_->remove_debug_counter(id);
  }

  sai_status_t _getAttribute(DebugCounterSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_debug_counter_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(DebugCounterSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_debug_counter_attribute(id, attr);
  }

  sai_debug_counter_api_t* api_;
  friend class SaiApi<DebugCounterApi>;
};
} // namespace facebook::fboss
