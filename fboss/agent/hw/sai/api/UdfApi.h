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

namespace facebook::fboss {

class UdfApi;

struct SaiUdfTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_UDF;
  using SaiApiT = UdfApi;
  struct Attributes {
    using EnumType = sai_udf_attr_t;
    using UdfMatchId =
        SaiAttribute<EnumType, SAI_UDF_ATTR_MATCH_ID, SaiObjectIdT>;
    using UdfGroupId =
        SaiAttribute<EnumType, SAI_UDF_ATTR_GROUP_ID, SaiObjectIdT>;
    using Base = SaiAttribute<EnumType, SAI_UDF_ATTR_BASE, sai_int32_t>;
    using Offset = SaiAttribute<EnumType, SAI_UDF_ATTR_OFFSET, sai_uint16_t>;
  };

  using AdapterKey = UdfSaiId;
  using CreateAttributes = std::tuple<
      Attributes::UdfMatchId,
      Attributes::UdfGroupId,
      std::optional<Attributes::Base>,
      Attributes::Offset>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(Udf, UdfMatchId);
SAI_ATTRIBUTE_NAME(Udf, UdfGroupId);
SAI_ATTRIBUTE_NAME(Udf, Base);
SAI_ATTRIBUTE_NAME(Udf, Offset);

class UdfApi : public SaiApi<UdfApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_UDF;
  UdfApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for udf api");
  }

  sai_status_t _create(
      UdfSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_udf(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(UdfSaiId id) const {
    return api_->remove_udf(id);
  }

  sai_status_t _getAttribute(UdfSaiId id, sai_attribute_t* attr) const {
    return api_->get_udf_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(UdfSaiId id, const sai_attribute_t* attr) const {
    return api_->set_udf_attribute(id, attr);
  }

  sai_udf_api_t* api_;
  friend class SaiApi<UdfApi>;
};

} // namespace facebook::fboss
