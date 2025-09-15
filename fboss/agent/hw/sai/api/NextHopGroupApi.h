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

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

#include <set>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class NextHopGroupApi;

namespace detail {
using NextHopMemberKey =
    std::pair<SaiNextHopTraits::AdapterHostKey, sai_uint32_t>; // weight
/* For NSF, due to the limited availability of ARS supported ECMP groups, the
 * ecmp resource manager does a reclaim of ARS groups when they become
 * available during a state/config update. Since the SAI SDK does not support
 * directly converting a non ARS ECMP group, the route update will set the new
 * ARS mode within SaiNextHopGroupKey.
 * There is no change to the nhop set though.
 *
 * Because the key changed, refOrEmplace operation would trigger a
 * new NHG group reference with the same nhop set as the existing ECMP.
 *
 * Having the ARS mode as part of the adapter host key allows SAI store to
 * create a new ECMP group within the ARS range while a non ARS ECMP still
 * exists with the same nhop set.
 */
struct NextHopGroupAdapterHostKey {
  std::set<NextHopMemberKey> nhopMemberSet = {};
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  sai_uint32_t mode = SAI_ARS_MODE_FIXED; // switching mode
#else
  sai_uint32_t mode = 0;
#endif
  bool operator==(const NextHopGroupAdapterHostKey& other) const {
    return nhopMemberSet == other.nhopMemberSet && mode == other.mode;
  }
  friend struct fmt::formatter<NextHopGroupAdapterHostKey>;
};
} // namespace detail

struct SaiNextHopGroupTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP;
  using SaiApiT = NextHopGroupApi;
  struct Attributes {
    using EnumType = sai_next_hop_group_attr_t;
    using NextHopMemberList = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST,
        std::vector<sai_object_id_t>>;
    using Type =
        SaiAttribute<EnumType, SAI_NEXT_HOP_GROUP_ATTR_TYPE, sai_int32_t>;
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    using ArsObjectId = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_ATTR_ARS_OBJECT_ID,
        sai_object_id_t,
        SaiObjectIdDefault>;
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
    using HashAlgorithm = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_ATTR_HASH_ALGORITHM,
        sai_int32_t,
        StdNullOptDefault<sai_int32_t>>;
#endif
    struct AttributeArsNextHopGroupMetaData {
      std::optional<sai_attr_id_t> operator()();
    };
    using ArsNextHopGroupMetaData = SaiExtensionAttribute<
        sai_uint32_t,
        AttributeArsNextHopGroupMetaData,
        SaiIntDefault<sai_uint32_t>>;
  };

  using AdapterKey = NextHopGroupSaiId;
  using AdapterHostKey = typename detail::NextHopGroupAdapterHostKey;
  using CreateAttributes = std::tuple<
      Attributes::Type
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      ,
      std::optional<Attributes::ArsObjectId>
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
      ,
      std::optional<Attributes::HashAlgorithm>
#endif
      >;
};

SAI_ATTRIBUTE_NAME(NextHopGroup, NextHopMemberList)
SAI_ATTRIBUTE_NAME(NextHopGroup, Type)
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
SAI_ATTRIBUTE_NAME(NextHopGroup, ArsObjectId)
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
SAI_ATTRIBUTE_NAME(NextHopGroup, HashAlgorithm)
#endif
SAI_ATTRIBUTE_NAME(NextHopGroup, ArsNextHopGroupMetaData)

struct SaiNextHopGroupMemberTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER;
  using SaiApiT = NextHopGroupApi;
  struct Attributes {
    using EnumType = sai_next_hop_group_member_attr_t;
    using NextHopId = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID,
        SaiObjectIdT>;
    using NextHopGroupId = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID,
        SaiObjectIdT>;
    using Weight = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_MEMBER_ATTR_WEIGHT,
        sai_uint32_t>;
  };

  using AdapterKey = NextHopGroupMemberSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::NextHopGroupId, Attributes::NextHopId>;
  using CreateAttributes = std::tuple<
      Attributes::NextHopGroupId,
      Attributes::NextHopId,
      std::optional<Attributes::Weight>>;
};

SAI_ATTRIBUTE_NAME(NextHopGroupMember, NextHopGroupId)
SAI_ATTRIBUTE_NAME(NextHopGroupMember, NextHopId)
SAI_ATTRIBUTE_NAME(NextHopGroupMember, Weight)

class NextHopGroupApi : public SaiApi<NextHopGroupApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_NEXT_HOP_GROUP;
  NextHopGroupApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for next hop group api");
  }
  NextHopGroupApi(const NextHopGroupApi& other) = delete;
  NextHopGroupApi& operator=(const NextHopGroupApi& other) = delete;

 private:
  sai_status_t _create(
      NextHopGroupSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_next_hop_group(
        rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _create(
      NextHopGroupMemberSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_next_hop_group_member(
        rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(NextHopGroupSaiId next_hop_group_id) const {
    return api_->remove_next_hop_group(next_hop_group_id);
  }
  sai_status_t _remove(NextHopGroupMemberSaiId next_hop_group_id) const {
    return api_->remove_next_hop_group_member(next_hop_group_id);
  }
  sai_status_t _getAttribute(NextHopGroupSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_next_hop_group_attribute(id, 1, attr);
  }
  sai_status_t _getAttribute(NextHopGroupMemberSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_next_hop_group_member_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(NextHopGroupSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_next_hop_group_attribute(id, attr);
  }
  sai_status_t _setAttribute(
      NextHopGroupMemberSaiId id,
      const sai_attribute_t* attr) const {
    return api_->set_next_hop_group_member_attribute(id, attr);
  }
  sai_status_t _bulkSetAttribute(
      NextHopGroupMemberSaiId* ids,
      const sai_attribute_t* attr,
      sai_status_t* retStatus,
      size_t objectCount) const {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    sai_object_id_t rawIds[objectCount];
    for (auto idx = 0; idx < objectCount; idx++) {
      rawIds[idx] = *rawSaiId(&ids[idx]);
    }
    return api_->set_next_hop_group_members_attribute(
        objectCount,
        rawIds,
        attr,
        SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR,
        retStatus);
#else
    return SAI_STATUS_NOT_SUPPORTED;
#endif
  }

  sai_status_t _bulkCreate(
      sai_object_id_t* ids,
      sai_status_t* retStatus,
      sai_object_id_t switch_id,
      uint32_t* attrCount,
      size_t objectCount,
      const sai_attribute_t** attr) const {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    return api_->create_next_hop_group_members(
        switch_id,
        objectCount,
        attrCount,
        attr,
        SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR,
        ids,
        retStatus);
#else
    return SAI_STATUS_NOT_SUPPORTED;
#endif
  }

  sai_status_t _bulkRemove(
      size_t objectCount,
      const NextHopGroupMemberSaiId* object_id,
      sai_status_t* retStatus) const {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    const unsigned long* objects =
        reinterpret_cast<const unsigned long*>(object_id);
    return api_->remove_next_hop_group_members(
        objectCount, objects, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, retStatus);
#else
    return SAI_STATUS_NOT_SUPPORTED;
#endif
  }

  sai_next_hop_group_api_t* api_;
  friend class SaiApi<NextHopGroupApi>;
};

} // namespace facebook::fboss

namespace fmt {
template <>
struct formatter<facebook::fboss::detail::NextHopGroupAdapterHostKey> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(
      const facebook::fboss::detail::NextHopGroupAdapterHostKey& ahk,
      FormatContext& ctx) const {
    return format_to(ctx.out(), "{}, {}", ahk.nhopMemberSet, ahk.mode);
  }
};
} // namespace fmt

namespace std {
template <>
struct hash<facebook::fboss::SaiNextHopGroupTraits::AdapterHostKey> {
  size_t operator()(
      const facebook::fboss::SaiNextHopGroupTraits::AdapterHostKey& k) const;
};
} // namespace std
