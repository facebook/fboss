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

namespace facebook::fboss {

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

SAI_ATTRIBUTE_NAME(Vlan, VlanId);
SAI_ATTRIBUTE_NAME(Vlan, MemberList);

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
SAI_ATTRIBUTE_NAME(VlanMember, BridgePortId);
SAI_ATTRIBUTE_NAME(VlanMember, VlanId);

class VlanApi : public SaiApi<VlanApi> {
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
      VlanSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_vlan(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _create(
      VlanMemberSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_vlan_member(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(VlanSaiId id) {
    return api_->remove_vlan(id);
  }
  sai_status_t _remove(VlanMemberSaiId id) {
    return api_->remove_vlan_member(id);
  }

  sai_status_t _getAttribute(VlanSaiId id, sai_attribute_t* attr) const {
    return api_->get_vlan_attribute(id, 1, attr);
  }
  sai_status_t _getAttribute(VlanMemberSaiId id, sai_attribute_t* attr) const {
    return api_->get_vlan_member_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(VlanSaiId id, const sai_attribute_t* attr) {
    return api_->set_vlan_attribute(id, attr);
  }
  sai_status_t _setAttribute(VlanMemberSaiId id, const sai_attribute_t* attr) {
    return api_->set_vlan_member_attribute(id, attr);
  }

  sai_vlan_api_t* api_;
  friend class SaiApi<VlanApi>;
};

} // namespace facebook::fboss
