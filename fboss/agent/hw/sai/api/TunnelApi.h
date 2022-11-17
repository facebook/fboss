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

namespace detail {

template <sai_tunnel_term_table_entry_type_t type>
struct SaiTunnelTermTraits;

template <>
struct SaiTunnelTermTraits<SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP> {
  using EnumType = sai_tunnel_term_table_entry_attr_t;
  struct Attributes {
    using Type = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TYPE,
        sai_int32_t>;
    using VrId = SaiAttribute<
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
      Attributes::VrId,
      Attributes::DstIp,
      Attributes::TunnelType,
      Attributes::ActionTunnelId>;
  using AdapterHostKey = CreateAttributes;
};

template <>
struct SaiTunnelTermTraits<SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2P> {
  using EnumType = sai_tunnel_term_table_entry_attr_t;
  struct Attributes {
    using Type = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TYPE,
        sai_int32_t>;
    using VrId = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_VR_ID,
        SaiObjectIdT>;
    using DstIp = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP,
        folly::IPAddress>;
    using DstIpMask = void;
    using SrcIp = SaiAttribute<
        EnumType,
        SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP,
        folly::IPAddress>;
    using SrcIpMask = void;
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
      Attributes::VrId,
      Attributes::DstIp,
      Attributes::SrcIp,
      Attributes::TunnelType,
      Attributes::ActionTunnelId>;
  using AdapterHostKey = CreateAttributes;
};

} // namespace detail

template <sai_tunnel_term_table_entry_type_t type>
struct SaiTunnelTermTraitsT {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY;
  using SaiApiT = TunnelApi;
  using AdapterKey = TunnelTermSaiId;
  using Attributes = typename detail::SaiTunnelTermTraits<type>::Attributes;
  using CreateAttributes =
      typename detail::SaiTunnelTermTraits<type>::CreateAttributes;
  using AdapterHostKey =
      typename detail::SaiTunnelTermTraits<type>::AdapterHostKey;
  using ConditionAttributes =
      std::tuple<typename detail::SaiTunnelTermTraits<type>::Attributes::Type>;
  inline const static ConditionAttributes kConditionAttributes{type};
};

using SaiP2MPTunnelTermTraits =
    SaiTunnelTermTraitsT<SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP>;
using SaiP2PTunnelTermTraits =
    SaiTunnelTermTraitsT<SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2P>;

template <>
struct SaiObjectHasConditionalAttributes<SaiP2MPTunnelTermTraits>
    : public std::true_type {};

template <>
struct SaiObjectHasConditionalAttributes<SaiP2PTunnelTermTraits>
    : public std::true_type {};

using SaiTunnelTermTraits =
    ConditionObjectTraits<SaiP2MPTunnelTermTraits, SaiP2PTunnelTermTraits>;
using SaiTunnelTermAdapterHostKey =
    typename SaiTunnelTermTraits::AdapterHostKey;
using SaiTunnelTermAdaptertKey =
    typename SaiTunnelTermTraits::AdapterKey<TunnelTermSaiId>;

SAI_ATTRIBUTE_NAME(P2MPTunnelTerm, Type);
SAI_ATTRIBUTE_NAME(P2MPTunnelTerm, VrId);
SAI_ATTRIBUTE_NAME(P2MPTunnelTerm, DstIp);
SAI_ATTRIBUTE_NAME(P2MPTunnelTerm, DstIpMask);
SAI_ATTRIBUTE_NAME(P2MPTunnelTerm, SrcIp);
SAI_ATTRIBUTE_NAME(P2MPTunnelTerm, SrcIpMask);
SAI_ATTRIBUTE_NAME(P2MPTunnelTerm, TunnelType);
SAI_ATTRIBUTE_NAME(P2MPTunnelTerm, ActionTunnelId);

class TunnelApi : public SaiApi<TunnelApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_TUNNEL;
  TunnelApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for tunnel api");
  }

 private:
  sai_status_t _create(
      TunnelSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_tunnel(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(TunnelSaiId key) const {
    return api_->remove_tunnel(key);
  }
  sai_status_t _getAttribute(TunnelSaiId key, sai_attribute_t* attr) const {
    return api_->get_tunnel_attribute(key, 1, attr);
  }
  sai_status_t _setAttribute(TunnelSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_tunnel_attribute(key, attr);
  }

  sai_status_t _getStats(
      TunnelSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    return api_->get_tunnel_stats(key, num_of_counters, counter_ids, counters);
    // #TODO: get_tunnel_stats will be deprecated
    // use get_tunnel_stats_ext(key, num_of_counters, counter_ids, mode,
    // counters) if supported later
  }

  sai_status_t _clearStats(
      TunnelSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_tunnel_stats(key, num_of_counters, counter_ids);
  }

  sai_status_t _create(
      TunnelTermSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_tunnel_term_table_entry(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(TunnelTermSaiId id) const {
    return api_->remove_tunnel_term_table_entry(id);
  }

  sai_status_t _getAttribute(TunnelTermSaiId key, sai_attribute_t* attr) const {
    return api_->get_tunnel_term_table_entry_attribute(key, 1, attr);
  }

  sai_status_t _setAttribute(TunnelTermSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_tunnel_term_table_entry_attribute(key, attr);
  }

  sai_tunnel_api_t* api_;
  friend class SaiApi<TunnelApi>;
};

} // namespace facebook::fboss
