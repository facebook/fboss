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

#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
class ArsApi;

struct SaiArsTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_ARS;
  using SaiApiT = ArsApi;
  struct Attributes {
    using EnumType = sai_ars_attr_t;
    using Mode = SaiAttribute<EnumType, SAI_ARS_ATTR_MODE, sai_int32_t>;
    using IdleTime = SaiAttribute<
        EnumType,
        SAI_ARS_ATTR_IDLE_TIME,
        sai_uint32_t,
        StdNullOptDefault<sai_uint32_t>>;
    using MaxFlows = SaiAttribute<
        EnumType,
        SAI_ARS_ATTR_MAX_FLOWS,
        sai_uint32_t,
        StdNullOptDefault<sai_uint32_t>>;
    using PrimaryPathQualityThreshold = SaiAttribute<
        EnumType,
        SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD,
        sai_uint32_t,
        StdNullOptDefault<sai_uint32_t>>;
    using AlternatePathCost = SaiAttribute<
        EnumType,
        SAI_ARS_ATTR_ALTERNATE_PATH_COST,
        sai_uint32_t,
        StdNullOptDefault<sai_uint32_t>>;
    using AlternatePathBias = SaiAttribute<
        EnumType,
        SAI_ARS_ATTR_ALTERNATE_PATH_BIAS,
        sai_uint32_t,
        StdNullOptDefault<sai_uint32_t>>;
  };

  using AdapterKey = ArsSaiId;
  using CreateAttributes = std::tuple<
      Attributes::Mode,
      std::optional<Attributes::IdleTime>,
      std::optional<Attributes::MaxFlows>,
      std::optional<Attributes::PrimaryPathQualityThreshold>,
      std::optional<Attributes::AlternatePathCost>,
      std::optional<Attributes::AlternatePathBias>>;
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
  using AdapterHostKey = std::tuple<
      std::optional<Attributes::AlternatePathCost>,
      std::optional<Attributes::AlternatePathBias>>;
#else
  using AdapterHostKey = std::monostate;
#endif
};

SAI_ATTRIBUTE_NAME(Ars, Mode)
SAI_ATTRIBUTE_NAME(Ars, IdleTime)
SAI_ATTRIBUTE_NAME(Ars, MaxFlows)
SAI_ATTRIBUTE_NAME(Ars, PrimaryPathQualityThreshold)
SAI_ATTRIBUTE_NAME(Ars, AlternatePathCost)
SAI_ATTRIBUTE_NAME(Ars, AlternatePathBias)

inline SaiArsTraits::AdapterHostKey getAdapterHostKey(
    const SaiArsTraits::CreateAttributes& createAttributes) {
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
  return SaiArsTraits::AdapterHostKey{
      std::get<std::optional<SaiArsTraits::Attributes::AlternatePathCost>>(
          createAttributes),
      std::get<std::optional<SaiArsTraits::Attributes::AlternatePathBias>>(
          createAttributes)};
#else
  return SaiArsTraits::AdapterHostKey{};
#endif
}

class ArsApi : public SaiApi<ArsApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_ARS;
  ArsApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for ars api");
  }
  ArsApi(const ArsApi& other) = delete;
  ArsApi& operator=(const ArsApi& other) = delete;

 private:
  sai_status_t _create(
      ArsSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_ars(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(ArsSaiId id) const {
    return api_->remove_ars(id);
  }
  sai_status_t _getAttribute(ArsSaiId id, sai_attribute_t* attr) const {
    return api_->get_ars_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(ArsSaiId id, const sai_attribute_t* attr) const {
    return api_->set_ars_attribute(id, attr);
  }

  sai_ars_api_t* api_;
  friend class SaiApi<ArsApi>;
};
#endif

} // namespace facebook::fboss
