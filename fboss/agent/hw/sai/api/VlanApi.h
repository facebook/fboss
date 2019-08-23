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
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

class VlanApi;

struct SaiVlanTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_VLAN;
  using SaiApiT = VlanApi;
  struct Attributes {
    using EnumType = sai_vlan_attr_t;
    using VlanId = SaiAttribute<EnumType, SAI_VLAN_ATTR_VLAN_ID, sai_uint16_t>;
    using MemberList = SaiAttribute<
        EnumType,
        SAI_VLAN_ATTR_MEMBER_LIST,
        std::vector<sai_object_id_t>>;
  };

  using AdapterKey = VlanSaiId;
  using AdapterHostKey = Attributes::VlanId;
  using CreateAttributes = std::tuple<Attributes::VlanId>;
};

struct SaiVlanMemberTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_VLAN_MEMBER;
  using SaiApiT = VlanApi;
  struct Attributes {
    using EnumType = sai_vlan_member_attr_t;
    using BridgePortId = SaiAttribute<
        EnumType,
        SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID,
        SaiObjectIdT>;
    using VlanId =
        SaiAttribute<EnumType, SAI_VLAN_MEMBER_ATTR_VLAN_ID, SaiObjectIdT>;
  };

  using AdapterKey = VlanMemberSaiId;
  using AdapterHostKey = Attributes::BridgePortId;
  using CreateAttributes =
      std::tuple<Attributes::VlanId, Attributes::BridgePortId>;
};

struct VlanApiParameters {
  static constexpr sai_api_t ApiType = SAI_API_VLAN;
  struct Attributes {
    using EnumType = sai_vlan_attr_t;
    using VlanId = SaiAttribute<EnumType, SAI_VLAN_ATTR_VLAN_ID, sai_uint16_t>;
    using MemberList = SaiAttribute<
        EnumType,
        SAI_VLAN_ATTR_MEMBER_LIST,
        std::vector<sai_object_id_t>>;
    using CreateAttributes = SaiAttributeTuple<VlanId>;
    Attributes(const CreateAttributes& attrs) {
      std::tie(vlanId) = attrs.value();
    }
    CreateAttributes attrs() const {
      return {vlanId};
    }
    bool operator==(const Attributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const Attributes& other) const {
      return !(*this == other);
    }
    typename VlanId::ValueType vlanId;
  };
  struct MemberAttributes {
    using EnumType = sai_vlan_member_attr_t;
    using BridgePortId = SaiAttribute<
        EnumType,
        SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID,
        SaiObjectIdT>;
    using VlanId =
        SaiAttribute<EnumType, SAI_VLAN_MEMBER_ATTR_VLAN_ID, SaiObjectIdT>;
    using CreateAttributes = SaiAttributeTuple<VlanId, BridgePortId>;
    MemberAttributes(const CreateAttributes& attrs) {
      std::tie(vlanId, bridgePortId) = attrs.value();
    }
    CreateAttributes attrs() const {
      return {vlanId, bridgePortId};
    }
    bool operator==(const MemberAttributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const MemberAttributes& other) const {
      return !(*this == other);
    }
    typename VlanId::ValueType vlanId;
    typename BridgePortId::ValueType bridgePortId;
  };
};

class VlanApi : public SaiApi<VlanApi, VlanApiParameters> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_VLAN;
  VlanApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for vlan api");
  }
  VlanApi(const VlanApi& other) = delete;

 private:
  sai_status_t _create(
      sai_object_id_t* vlan_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_vlan(vlan_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t vlan_id) {
    return api_->remove_vlan(vlan_id);
  }

  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t handle) const {
    return api_->get_vlan_attribute(handle, 1, attr);
  }
  sai_status_t _setAttr(const sai_attribute_t* attr, sai_object_id_t handle) {
    return api_->set_vlan_attribute(handle, attr);
  }
  sai_status_t _createMember(
      sai_object_id_t* vlan_member_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_vlan_member(
        vlan_member_id, switch_id, count, attr_list);
  }
  sai_status_t _removeMember(sai_object_id_t vlan_member_id) {
    return api_->remove_vlan_member(vlan_member_id);
  }
  sai_status_t _getMemberAttr(sai_attribute_t* attr, sai_object_id_t handle)
      const {
    return api_->get_vlan_member_attribute(handle, 1, attr);
  }
  sai_status_t _setMemberAttr(
      const sai_attribute_t* attr,
      sai_object_id_t handle) {
    return api_->set_vlan_member_attribute(handle, attr);
  }
  sai_vlan_api_t* api_;
  friend class SaiApi<VlanApi, VlanApiParameters>;
};

} // namespace fboss
} // namespace facebook
