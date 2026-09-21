// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#if defined(BRCM_SAI_SDK_XGS_GTE_16_0)

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class VirtualChannelApi;

struct SaiVirtualChannelTraits {
  static constexpr sai_api_t ApiType = SAI_API_VIRTUAL_CHANNEL;
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_VIRTUAL_CHANNEL;
  using SaiApiT = VirtualChannelApi;
  struct Attributes {
    using EnumType = sai_virtual_channel_attr_t;
    using Port =
        SaiAttribute<EnumType, SAI_VIRTUAL_CHANNEL_ATTR_PORT, SaiObjectIdT>;
    using Index =
        SaiAttribute<EnumType, SAI_VIRTUAL_CHANNEL_ATTR_INDEX, sai_uint8_t>;
    using CbfcSenderCreditProfile = SaiAttribute<
        EnumType,
        SAI_VIRTUAL_CHANNEL_ATTR_CBFC_SENDER_CREDIT_PROFILE,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using CbfcReceiverEnable = SaiAttribute<
        EnumType,
        SAI_VIRTUAL_CHANNEL_ATTR_CBFC_RECEIVER_ENABLE,
        bool,
        SaiBoolDefaultFalse>;
    using CbfcSenderEnable = SaiAttribute<
        EnumType,
        SAI_VIRTUAL_CHANNEL_ATTR_CBFC_SENDER_ENABLE,
        bool,
        SaiBoolDefaultFalse>;
  };
  using AdapterKey = VirtualChannelSaiId;
  using AdapterHostKey = std::tuple<Attributes::Port, Attributes::Index>;
  using CreateAttributes = std::tuple<
      Attributes::Port,
      Attributes::Index,
      std::optional<Attributes::CbfcSenderCreditProfile>,
      std::optional<Attributes::CbfcReceiverEnable>,
      std::optional<Attributes::CbfcSenderEnable>>;
};

SAI_ATTRIBUTE_NAME(VirtualChannel, Port);
SAI_ATTRIBUTE_NAME(VirtualChannel, Index);
SAI_ATTRIBUTE_NAME(VirtualChannel, CbfcSenderCreditProfile);
SAI_ATTRIBUTE_NAME(VirtualChannel, CbfcReceiverEnable);
SAI_ATTRIBUTE_NAME(VirtualChannel, CbfcSenderEnable);

class VirtualChannelApi : public SaiApi<VirtualChannelApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_VIRTUAL_CHANNEL;
  VirtualChannelApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(
        status, ApiType, "Failed to query for virtual channel api");
  }
  VirtualChannelApi(const VirtualChannelApi& other) = delete;
  VirtualChannelApi& operator=(const VirtualChannelApi& other) = delete;

 private:
  sai_status_t _create(
      VirtualChannelSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_virtual_channel(
        rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(VirtualChannelSaiId id) const {
    return api_->remove_virtual_channel(id);
  }
  sai_status_t _getAttribute(VirtualChannelSaiId key, sai_attribute_t* attr)
      const {
    return api_->get_virtual_channel_attribute(key, 1, attr);
  }
  sai_status_t _setAttribute(
      VirtualChannelSaiId key,
      const sai_attribute_t* attr) const {
    return api_->set_virtual_channel_attribute(key, attr);
  }

  sai_virtual_channel_api_t* api_;
  friend class SaiApi<VirtualChannelApi>;
};

} // namespace facebook::fboss

#endif
