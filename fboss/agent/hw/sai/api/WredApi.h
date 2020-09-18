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

class WredApi;

struct SaiWredTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_WRED;
  using SaiApiT = WredApi;
  struct Attributes {
    using EnumType = sai_wred_attr_t;

    /* Wred */
    using GreenEnable =
        SaiAttribute<EnumType, SAI_WRED_ATTR_GREEN_ENABLE, bool>;
    using GreenMinThreshold =
        SaiAttribute<EnumType, SAI_WRED_ATTR_GREEN_MIN_THRESHOLD, sai_uint32_t>;
    using GreenMaxThreshold =
        SaiAttribute<EnumType, SAI_WRED_ATTR_GREEN_MAX_THRESHOLD, sai_uint32_t>;

    /* ECN */
    using EcnMarkMode =
        SaiAttribute<EnumType, SAI_WRED_ATTR_ECN_MARK_MODE, sai_int32_t>;
    using EcnGreenMinThreshold = SaiAttribute<
        EnumType,
        SAI_WRED_ATTR_ECN_GREEN_MIN_THRESHOLD,
        sai_uint32_t>;
    using EcnGreenMaxThreshold = SaiAttribute<
        EnumType,
        SAI_WRED_ATTR_ECN_GREEN_MAX_THRESHOLD,
        sai_uint32_t>;
  };

  using AdapterKey = WredSaiId;
  using CreateAttributes = std::tuple<
      Attributes::GreenEnable,
      std::optional<Attributes::GreenMinThreshold>,
      std::optional<Attributes::GreenMaxThreshold>,
      Attributes::EcnMarkMode,
      std::optional<Attributes::EcnGreenMinThreshold>,
      std::optional<Attributes::EcnGreenMaxThreshold>>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(Wred, GreenEnable);
SAI_ATTRIBUTE_NAME(Wred, GreenMinThreshold);
SAI_ATTRIBUTE_NAME(Wred, GreenMaxThreshold);
SAI_ATTRIBUTE_NAME(Wred, EcnMarkMode);
SAI_ATTRIBUTE_NAME(Wred, EcnGreenMinThreshold);
SAI_ATTRIBUTE_NAME(Wred, EcnGreenMaxThreshold);

class WredApi : public SaiApi<WredApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_WRED;
  WredApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for wred api");
  }

  sai_status_t _create(
      WredSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_wred(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(WredSaiId id) {
    return api_->remove_wred(id);
  }

  sai_status_t _getAttribute(WredSaiId id, sai_attribute_t* attr) const {
    return api_->get_wred_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(WredSaiId id, const sai_attribute_t* attr) {
    return api_->set_wred_attribute(id, attr);
  }

  sai_wred_api_t* api_;
  friend class SaiApi<WredApi>;
};

} // namespace facebook::fboss
