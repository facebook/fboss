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
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>

extern "C" {
#include <sai.h>
}

bool operator==(
    const sai_qos_map_params_t& lhs,
    const sai_qos_map_params_t& rhs);
bool operator==(const sai_qos_map_t& lhs, const sai_qos_map_t& rhs);
bool operator!=(
    const sai_qos_map_params_t& lhs,
    const sai_qos_map_params_t& rhs);
bool operator!=(const sai_qos_map_t& lhs, const sai_qos_map_t& rhs);

namespace facebook::fboss {

class QosMapApi;

struct SaiQosMapTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_QOS_MAP;
  using SaiApiT = QosMapApi;
  struct Attributes {
    using EnumType = sai_qos_map_attr_t;
    using Type = SaiAttribute<EnumType, SAI_QOS_MAP_ATTR_TYPE, sai_int32_t>;
    using MapToValueList = SaiAttribute<
        EnumType,
        SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST,
        std::vector<sai_qos_map_t>>;
  };

  using AdapterKey = QosMapSaiId;
  using AdapterHostKey = std::tuple<Attributes::Type>;
  using CreateAttributes =
      std::tuple<Attributes::Type, Attributes::MapToValueList>;
};

SAI_ATTRIBUTE_NAME(QosMap, Type)
SAI_ATTRIBUTE_NAME(QosMap, MapToValueList)

class QosMapApi : public SaiApi<QosMapApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_QOS_MAP;
  QosMapApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for qos map api");
  }

 private:
  sai_status_t _create(
      QosMapSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_qos_map(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(QosMapSaiId id) {
    return api_->remove_qos_map(id);
  }
  sai_status_t _getAttribute(QosMapSaiId id, sai_attribute_t* attr) const {
    return api_->get_qos_map_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(QosMapSaiId id, const sai_attribute_t* attr) {
    return api_->set_qos_map_attribute(id, attr);
  }

  sai_qos_map_api_t* api_;
  friend class SaiApi<QosMapApi>;
};

} // namespace facebook::fboss
