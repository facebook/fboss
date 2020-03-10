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
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <set>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class NextHopGroupApi;

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
  };

  using AdapterKey = NextHopGroupSaiId;
  using NextHopMemberKey = SaiNextHopTraits::AdapterHostKey;
  using AdapterHostKey = std::set<NextHopMemberKey>;
  using CreateAttributes = std::tuple<Attributes::Type>;
};

SAI_ATTRIBUTE_NAME(NextHopGroup, NextHopMemberList)
SAI_ATTRIBUTE_NAME(NextHopGroup, Type)

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
      sai_attribute_t* attr_list) {
    return api_->create_next_hop_group(
        rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _create(
      NextHopGroupMemberSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_next_hop_group_member(
        rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(NextHopGroupSaiId next_hop_group_id) {
    return api_->remove_next_hop_group(next_hop_group_id);
  }
  sai_status_t _remove(NextHopGroupMemberSaiId next_hop_group_id) {
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
  sai_status_t _setAttribute(
      NextHopGroupSaiId id,
      const sai_attribute_t* attr) {
    return api_->set_next_hop_group_attribute(id, attr);
  }
  sai_status_t _setAttribute(
      NextHopGroupMemberSaiId id,
      const sai_attribute_t* attr) {
    return api_->set_next_hop_group_member_attribute(id, attr);
  }

  sai_next_hop_group_api_t* api_;
  friend class SaiApi<NextHopGroupApi>;
};

} // namespace facebook::fboss

namespace std {
template <>
struct hash<facebook::fboss::SaiNextHopGroupTraits::AdapterHostKey> {
  size_t operator()(
      const facebook::fboss::SaiNextHopGroupTraits::AdapterHostKey& k) const;
};
} // namespace std
