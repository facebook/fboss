// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)

extern "C" {
#include <sai.h>
#include <saiextensions.h>
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentalvendorswitch.h>
#else
#include <saiexperimentalvendorswitch.h>
#endif
}

namespace facebook::fboss {

class VendorSwitchApi;

struct SaiVendorSwitchTraits {
  // IMPORTANT: Cast to sai_object_type_t so that all our infrastructure works.
  static constexpr sai_object_type_t ObjectType =
      static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_VENDOR_SWITCH);
  using SaiApiT = VendorSwitchApi;
  struct Attributes {
    using EnumType = sai_vendor_switch_attr_t;
    using EventList = SaiAttribute<
        EnumType,
        SAI_VENDOR_SWITCH_ATTR_EVENT_LIST,
        std::vector<sai_map_t>,
        SaiListDefault<sai_map_list_t>>;
    using DisableEventList = SaiAttribute<
        EnumType,
        SAI_VENDOR_SWITCH_ATTR_INCR_DISABLE_EVENT_LIST,
        std::vector<uint32_t>,
        SaiU32ListDefault>;
    using EnableEventList = SaiAttribute<
        EnumType,
        SAI_VENDOR_SWITCH_ATTR_INCR_ENABLE_EVENT_LIST,
        std::vector<uint32_t>,
        SaiU32ListDefault>;
    using CgmRejectStatusBitmap = SaiAttribute<
        EnumType,
        SAI_VENDOR_SWITCH_ATTR_CGM_REJECT_STATUS_BITMAP,
        sai_uint64_t>;
  };
  using AdapterKey = VendorSwitchSaiId;
  //  Need a single vendor switch object
  using AdapterHostKey = std::monostate;
  using CreateAttributes = std::tuple<Attributes::EventList>;
};

SAI_ATTRIBUTE_NAME(VendorSwitch, EventList)
SAI_ATTRIBUTE_NAME(VendorSwitch, DisableEventList)
SAI_ATTRIBUTE_NAME(VendorSwitch, EnableEventList)
SAI_ATTRIBUTE_NAME(VendorSwitch, CgmRejectStatusBitmap)

class VendorSwitchApi : public SaiApi<VendorSwitchApi> {
 public:
  static constexpr sai_api_t ApiType =
      static_cast<sai_api_t>(SAI_API_VENDOR_SWITCH);
  VendorSwitchApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for vendor switch api");
  }

 private:
  sai_status_t _create(
      VendorSwitchSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_vendor_switch(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(VendorSwitchSaiId id) const {
    return api_->remove_vendor_switch(id);
  }

  sai_status_t _getAttribute(VendorSwitchSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_vendor_switch_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(VendorSwitchSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_vendor_switch_attribute(id, attr);
  }

  sai_vendor_switch_api_t* api_;
  friend class SaiApi<VendorSwitchApi>;
};

} // namespace facebook::fboss

#endif
