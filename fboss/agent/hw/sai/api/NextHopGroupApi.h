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

namespace facebook {
namespace fboss {

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
  // vector of rifId, ip address pairs
  using AdapterHostKey = std::set<typename SaiNextHopTraits::AdapterHostKey>;
  using CreateAttributes = std::tuple<Attributes::Type>;
};

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
  };

  using AdapterKey = NextHopGroupMemberSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::NextHopGroupId, Attributes::NextHopId>;
  using CreateAttributes =
      std::tuple<Attributes::NextHopGroupId, Attributes::NextHopId>;
};

struct NextHopGroupApiParameters {
  static constexpr sai_api_t ApiType = SAI_API_NEXT_HOP_GROUP;
  struct Attributes {
    using EnumType = sai_next_hop_group_attr_t;
    using NextHopMemberList = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST,
        std::vector<sai_object_id_t>>;
    using Type =
        SaiAttribute<EnumType, SAI_NEXT_HOP_GROUP_ATTR_TYPE, sai_int32_t>;
    using CreateAttributes = SaiAttributeTuple<Type>;
    Attributes(const CreateAttributes& attrs) {
      std::tie(type) = attrs.value();
    }
    CreateAttributes attrs() const {
      return {type};
    }
    bool operator==(const Attributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const Attributes& other) const {
      return !(*this == other);
    }
    Type::ValueType type;
  };
  struct MemberAttributes {
    using EnumType = sai_next_hop_group_member_attr_t;
    using NextHopId = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID,
        SaiObjectIdT>;
    using NextHopGroupId = SaiAttribute<
        EnumType,
        SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID,
        SaiObjectIdT>;
    using CreateAttributes = SaiAttributeTuple<NextHopGroupId, NextHopId>;
    MemberAttributes(const CreateAttributes& attrs) {
      std::tie(nextHopGroupId, nextHopId) = attrs.value();
    }
    CreateAttributes attrs() const {
      return {nextHopGroupId, nextHopId};
    }
    bool operator==(const MemberAttributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const MemberAttributes& other) const {
      return !(*this == other);
    }
    NextHopGroupId::ValueType nextHopGroupId;
    NextHopId::ValueType nextHopId;
  };
};

class NextHopGroupApi
    : public SaiApi<NextHopGroupApi, NextHopGroupApiParameters> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_NEXT_HOP_GROUP;
  NextHopGroupApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for next hop group api");
  }
  NextHopGroupApi(const NextHopGroupApi& other) = delete;

 private:
  sai_status_t _create(
      sai_object_id_t* next_hop_group_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_next_hop_group(
        next_hop_group_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t next_hop_group_id) {
    return api_->remove_next_hop_group(next_hop_group_id);
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t handle) const {
    return api_->get_next_hop_group_attribute(handle, 1, attr);
  }
  sai_status_t _setAttr(const sai_attribute_t* attr, sai_object_id_t handle) {
    return api_->set_next_hop_group_attribute(handle, attr);
  }
  sai_status_t _createMember(
      sai_object_id_t* next_hop_group_member_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_next_hop_group_member(
        next_hop_group_member_id, switch_id, count, attr_list);
  }
  sai_status_t _removeMember(sai_object_id_t next_hop_group_member_id) {
    return api_->remove_next_hop_group_member(next_hop_group_member_id);
  }
  sai_status_t _getMemberAttr(sai_attribute_t* attr, sai_object_id_t handle)
      const {
    return api_->get_next_hop_group_member_attribute(handle, 1, attr);
  }
  sai_status_t _setMemberAttr(
      const sai_attribute_t* attr,
      sai_object_id_t handle) {
    return api_->set_next_hop_group_member_attribute(handle, attr);
  }
  sai_next_hop_group_api_t* api_;
  friend class SaiApi<NextHopGroupApi, NextHopGroupApiParameters>;
};

} // namespace fboss
} // namespace facebook

namespace std {
template <>
struct hash<facebook::fboss::SaiNextHopGroupTraits::AdapterHostKey> {
  size_t operator()(
      const facebook::fboss::SaiNextHopGroupTraits::AdapterHostKey& k) const;
};
} // namespace std
