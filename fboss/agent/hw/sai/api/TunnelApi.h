// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiDefaultAttributeValues.h"
#include "fboss/agent/hw/sai/api/Types.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class TunnelApi;

struct SaiTunnelTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_TUNNEL;
  using SaiApiT = TunnelApi;
  struct Attributes {
    using EnumType = sai_tunnel_attr_t;
    using Type = SaiAttribute<EnumType, SAI_TUNNEL_ATTR_TYPE, sai_int32_t>;
    using UnderlayInterface = SaiAttribute<
        EnumType,
        SAI_TUNNEL_ATTR_UNDERLAY_INTERFACE,
        SaiObjectIdT>;
    using OverlayInterface =
        SaiAttribute<EnumType, SAI_TUNNEL_ATTR_OVERLAY_INTERFACE, SaiObjectIdT>;
    using DecapTtlMode =
        SaiAttribute<EnumType, SAI_TUNNEL_ATTR_DECAP_TTL_MODE, sai_int32_t>;
    using DecapDscpMode =
        SaiAttribute<EnumType, SAI_TUNNEL_ATTR_DECAP_DSCP_MODE, sai_int32_t>;
    using DecapEcnMode =
        SaiAttribute<EnumType, SAI_TUNNEL_ATTR_DECAP_ECN_MODE, sai_int32_t>;
  };
  using AdapterKey = TunnelSaiId;
  using CreateAttributes = std::tuple<
      Attributes::Type,
      Attributes::UnderlayInterface,
      Attributes::OverlayInterface,
      std::optional<Attributes::DecapTtlMode>,
      std::optional<Attributes::DecapDscpMode>,
      std::optional<Attributes::DecapEcnMode>>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(Tunnel, Type);
SAI_ATTRIBUTE_NAME(Tunnel, UnderlayInterface);
SAI_ATTRIBUTE_NAME(Tunnel, OverlayInterface);
SAI_ATTRIBUTE_NAME(Tunnel, DecapTtlMode);
SAI_ATTRIBUTE_NAME(Tunnel, DecapDscpMode);
SAI_ATTRIBUTE_NAME(Tunnel, DecapEcnMode);

struct SaiTunnelTermTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY;
  using SaiApiT = TunnelApi;
  struct Attributes {
    using EnumType = sai_tunnel_term_table_entry_attr_t;
    using Type = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TYPE,
        sai_int32_t>;
    using EntryAttrVrId = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_VR_ID,
        SaiObjectIdT>;
    using DstIp = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP,
        folly::IPAddress>;
    using DstIpMask = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP_MASK,
        folly::IPAddress>;
    using SrcIp = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP,
        folly::IPAddress>;
    using SrcIpMask = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP_MASK,
        folly::IPAddress>;
    using TunnelType = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TUNNEL_TYPE,
        sai_int32_t>;
    using ActionTunnelId = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_ACTION_TUNNEL_ID,
        SaiObjectIdT>;
  };
  using AdapterKey = TunnelTermSaiId;
  using CreateAttributes = std::tuple<
      Attributes::Type,
      Attributes::EntryAttrVrId,
      Attributes::DstIp,
      Attributes::SrcIp,
      Attributes::TunnelType,
      Attributes::ActionTunnelId>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(TunnelTerm, Type);
SAI_ATTRIBUTE_NAME(TunnelTerm, EntryAttrVrId);
SAI_ATTRIBUTE_NAME(TunnelTerm, DstIp);
SAI_ATTRIBUTE_NAME(TunnelTerm, DstIpMask);
SAI_ATTRIBUTE_NAME(TunnelTerm, SrcIp);
SAI_ATTRIBUTE_NAME(TunnelTerm, SrcIpMask);
SAI_ATTRIBUTE_NAME(TunnelTerm, TunnelType);
SAI_ATTRIBUTE_NAME(TunnelTerm, ActionTunnelId);

class TunnelApi : public SaiApi<TunnelApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_TUNNEL;
  TunnelApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for tunnel api");
  }

 private:
  sai_tunnel_api_t* api_;
  friend class SaiApi<TunnelApi>;
};

} // namespace facebook::fboss
