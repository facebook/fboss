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
#include "fboss/agent/hw/sai/api/SaiDefaultAttributeValues.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

namespace detail {

template <sai_next_hop_type_t type>
struct NextHopAttributesTypes;

template <>
struct NextHopAttributesTypes<SAI_NEXT_HOP_TYPE_MPLS> {
  using LabelStack = facebook::fboss::SaiAttribute<
      sai_next_hop_attr_t,
      SAI_NEXT_HOP_ATTR_LABELSTACK,
      std::vector<sai_uint32_t>>;
};

template <>
struct NextHopAttributesTypes<SAI_NEXT_HOP_TYPE_IP> {
  using LabelStack = void;
};

template <typename Attributes, sai_next_hop_type_t type>
struct NextHopTraitsAttributes;

template <typename Attributes>
struct NextHopTraitsAttributes<Attributes, SAI_NEXT_HOP_TYPE_MPLS> {
  using AdapterHostKey = std::tuple<
      typename Attributes::RouterInterfaceId,
      typename Attributes::Ip,
      typename Attributes::LabelStack>;
  using CreateAttributes = std::tuple<
      typename Attributes::Type,
      typename Attributes::RouterInterfaceId,
      typename Attributes::Ip,
      typename Attributes::LabelStack,
      std::optional<typename Attributes::DisableTtlDecrement>>;
};

template <typename Attributes>
struct NextHopTraitsAttributes<Attributes, SAI_NEXT_HOP_TYPE_IP> {
  using AdapterHostKey = std::
      tuple<typename Attributes::RouterInterfaceId, typename Attributes::Ip>;
  using CreateAttributes = std::tuple<
      typename Attributes::Type,
      typename Attributes::RouterInterfaceId,
      typename Attributes::Ip,
      std::optional<typename Attributes::DisableTtlDecrement>>;
};
} // namespace detail

class NextHopApi;

/*
 * Next hop object has condition attribute SAI_NEXT_HOP_ATTR_TYPE.
 *
 * Accordingly SaiNextHopTraitsT is a template with next hop entry type as
 * template parameter.
 *
 * The template parameter governs structure of mandatory create attributes
 * trait(CreateAttributes) and adapter host key trait(AdapterHostKey).
 *
 * In particular, only next hop entry of type SAI_NEXT_HOP_TYPE_MPLS must have
 * SAI_NEXT_HOP_ATTR_LABELSTACK. Different next hop entries of type
 * SAI_NEXT_HOP_TYPE_MPLS type to the same ip over same interface can have
 * different SAI_NEXT_HOP_ATTR_LABELSTACK.
 * That makes SAI_NEXT_HOP_ATTR_LABELSTACK a part  of both  CreateAttributes and
 * AdapterHostKey trait. However This attribute is not available for next hop
 * of type SAI_NEXT_HOP_TYPE_IP. So both CreateAttributes and AdapterHostKey
 * trait for this type of next hop entry can not have this attribute.
 *
 * These different definitions of CreateAttributes and AdapterHostKey are
 * provided NextHopTraitsAttributes metafunction. This will prevent accidental
 * setting of label stack for unsupported next hop type. Such violations will be
 * detected at compile time.
 *
 * Further all specializations of object traits with condition attribute must
 * specialize trait SaiObjectHasConditionalAttributes as true_type.
 *
 * Object traits with condition attribute must provide a trait of
 * ConditionAttributes. This trait is a tuple of condition attributes.
 *
 * For next hop object traits, it is a tuple with single item Type
 *
 * Object traits with condition attribute must provide a trait of
 * isConditionMet. This trait is a unary predicate which returns true for
 * condition attribute with attribute expected by object trait.
 *
 * ConditionAttributes and  isConditionMet are used in warm boot processing to
 * filter away objects which do not have values expected by condition.
 *
 * In this particular case, isConditionMet simply compares type of discovered
 * next hop object (0th element of args) with that of next hop trait.
 */

template <sai_next_hop_type_t type>
struct SaiNextHopTraitsT {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_NEXT_HOP;
  using SaiApiT = NextHopApi;
  struct Attributes {
    using EnumType = sai_next_hop_attr_t;
    using Ip = SaiAttribute<EnumType, SAI_NEXT_HOP_ATTR_IP, folly::IPAddress>;
    using RouterInterfaceId = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID,
        SaiObjectIdT>;
    using LabelStack =
        typename detail::NextHopAttributesTypes<type>::LabelStack;
    using Type = SaiAttribute<EnumType, SAI_NEXT_HOP_ATTR_TYPE, sai_int32_t>;
    using DisableTtlDecrement = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_ATTR_DISABLE_DECREMENT_TTL,
        bool,
        SaiBoolDefaultFalse>;
  };
  using AdapterKey = NextHopSaiId;
  using AdapterHostKey = typename detail::
      NextHopTraitsAttributes<Attributes, type>::AdapterHostKey;
  using CreateAttributes = typename detail::
      NextHopTraitsAttributes<Attributes, type>::CreateAttributes;
  using ConditionAttributes = std::tuple<typename Attributes::Type>;
  inline const static ConditionAttributes kConditionAttributes{type};
};

using SaiIpNextHopTraits = SaiNextHopTraitsT<SAI_NEXT_HOP_TYPE_IP>;
using SaiMplsNextHopTraits = SaiNextHopTraitsT<SAI_NEXT_HOP_TYPE_MPLS>;
template <>
struct SaiObjectHasConditionalAttributes<SaiIpNextHopTraits>
    : public std::true_type {};
template <>
struct SaiObjectHasConditionalAttributes<SaiMplsNextHopTraits>
    : public std::true_type {};

using SaiNextHopTraits =
    ConditionObjectTraits<SaiIpNextHopTraits, SaiMplsNextHopTraits>;
using SaiNextHopAdapterHostKey = typename SaiNextHopTraits::AdapterHostKey;
using SaiNextHopAdaptertKey =
    typename SaiNextHopTraits::AdapterKey<NextHopSaiId>;

SAI_ATTRIBUTE_NAME(IpNextHop, Type)
SAI_ATTRIBUTE_NAME(IpNextHop, RouterInterfaceId)
SAI_ATTRIBUTE_NAME(IpNextHop, Ip)
SAI_ATTRIBUTE_NAME(MplsNextHop, LabelStack)
SAI_ATTRIBUTE_NAME(IpNextHop, DisableTtlDecrement);

class NextHopApi : public SaiApi<NextHopApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_NEXT_HOP;
  NextHopApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for next hop api");
  }

 private:
  sai_status_t _create(
      NextHopSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_next_hop(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(NextHopSaiId next_hop_id) const {
    return api_->remove_next_hop(next_hop_id);
  }
  sai_status_t _getAttribute(NextHopSaiId id, sai_attribute_t* attr) const {
    return api_->get_next_hop_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(NextHopSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_next_hop_attribute(id, attr);
  }

  sai_next_hop_api_t* api_;
  friend class SaiApi<NextHopApi>;
};

} // namespace facebook::fboss
