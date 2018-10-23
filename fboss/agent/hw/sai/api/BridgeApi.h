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

#include "SaiApi.h"
#include "SaiAttribute.h"

#include <folly/logging/xlog.h>

#include <boost/variant.hpp>

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

struct BridgeTypes {
  struct Attributes {
    using EnumType = sai_bridge_attr_t;
    using PortList = SaiAttribute<
        EnumType,
        SAI_BRIDGE_ATTR_PORT_LIST,
        sai_object_list_t,
        std::vector<sai_object_id_t>>;
  };
  using AttributeType = boost::variant<Attributes::PortList>;
  struct MemberAttributes {
    using EnumType = sai_bridge_port_attr_t;
    using BridgeId = SaiAttribute<
        EnumType,
        SAI_BRIDGE_PORT_ATTR_BRIDGE_ID,
        sai_object_id_t,
        SaiObjectIdT>;
    using PortId = SaiAttribute<
        EnumType,
        SAI_BRIDGE_PORT_ATTR_PORT_ID,
        sai_object_id_t,
        SaiObjectIdT>;
    using Type = SaiAttribute<EnumType, SAI_BRIDGE_PORT_ATTR_TYPE, int32_t>;
  };
  using MemberAttributeType = boost::variant<
      MemberAttributes::BridgeId,
      MemberAttributes::PortId,
      MemberAttributes::Type>;
  struct EntryType {};
};

class BridgeApi : public SaiApi<BridgeApi, BridgeTypes> {
 public:
  BridgeApi() {
    sai_status_t res = sai_api_query(SAI_API_BRIDGE, (void**)&api_);
    if (res != SAI_STATUS_SUCCESS) {
      throw SaiApiError(res);
    }
  }
 private:
  sai_status_t _create(
      sai_object_id_t* bridge_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_bridge(bridge_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t bridge_id) {
    return api_->remove_bridge(bridge_id);
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t handle) const {
    return api_->get_bridge_attribute(handle, 1, attr);
  }
  sai_status_t _setAttr(const sai_attribute_t* attr, sai_object_id_t handle) {
    return api_->set_bridge_attribute(handle, attr);
  }
  sai_status_t _getMemberAttr(sai_attribute_t* attr, sai_object_id_t handle)
      const {
    return api_->get_bridge_port_attribute(handle, 1, attr);
  }
  sai_status_t _setMemberAttr(
      const sai_attribute_t* attr,
      sai_object_id_t handle) {
    return api_->set_bridge_port_attribute(handle, attr);
  }
  sai_status_t _createMember(
      sai_object_id_t* bridge_port_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_bridge_port(
        bridge_port_id, switch_id, count, attr_list);
  }
  sai_status_t _removeMember(sai_object_id_t bridge_port_id) {
    return api_->remove_bridge_port(bridge_port_id);
  }
  sai_bridge_api_t* api_;
  friend class SaiApi<BridgeApi, BridgeTypes>;
};

} // namespace fboss
} // namespace facebook
