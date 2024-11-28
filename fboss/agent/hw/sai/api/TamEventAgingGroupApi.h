// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

extern "C" {
#include <sai.h>
#include <saiextensions.h>
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentaltameventaginggroup.h>
#else
#include <saiexperimentaltameventaginggroup.h>
#endif
}

namespace facebook::fboss {

class TamEventAgingGroupApi;

struct SaiTamEventAgingGroupTraits {
  // IMPORTANT: Cast to sai_object_type_t so that all our infrastructure works.
  static constexpr sai_object_type_t ObjectType =
      static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_TAM_EVENT_AGING_GROUP);

  using SaiApiT = TamEventAgingGroupApi;
  struct Attributes {
    using EnumType = sai_tam_event_aging_group_attr_t;
    using Type = SaiAttribute<
        EnumType,
        SAI_TAM_EVENT_AGING_GROUP_ATTR_TYPE,
        sai_int32_t>;
    using AgingTime = SaiAttribute<
        EnumType,
        SAI_TAM_EVENT_AGING_GROUP_ATTR_AGING_TIME,
        sai_uint16_t>; // in microseconds
  };
  using AdapterKey = TamEventAgingGroupSaiId;
  using AdapterHostKey = std::tuple<Attributes::Type, Attributes::AgingTime>;
  using CreateAttributes = AdapterHostKey;
};

SAI_ATTRIBUTE_NAME(TamEventAgingGroup, Type)
SAI_ATTRIBUTE_NAME(TamEventAgingGroup, AgingTime)

class TamEventAgingGroupApi : public SaiApi<TamEventAgingGroupApi> {
 public:
  static constexpr sai_api_t ApiType =
      static_cast<sai_api_t>(SAI_API_TAM_EVENT_AGING_GROUP);
  TamEventAgingGroupApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(
        status, ApiType, "Failed to query for TAM event aging group api");
  }

 private:
  // TAM Event Aging Group
  sai_status_t _create(
      TamEventAgingGroupSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_tam_event_aging_group(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(TamEventAgingGroupSaiId id) const {
    return api_->remove_tam_event_aging_group(id);
  }

  sai_status_t _getAttribute(TamEventAgingGroupSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_tam_event_aging_group_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(
      TamEventAgingGroupSaiId id,
      const sai_attribute_t* attr) const {
    return api_->set_tam_event_aging_group_attribute(id, attr);
  }

  sai_tam_event_aging_group_api_t* api_;
  friend class SaiApi<TamEventAgingGroupApi>;
};

} // namespace facebook::fboss

#endif
