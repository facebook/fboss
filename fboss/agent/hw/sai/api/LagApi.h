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

struct LagTypes {
  struct Attributes {
    using EnumType = sai_lag_attr_t;
    using MemberList = SaiAttribute<
        EnumType,
        SAI_LAG_ATTR_PORT_LIST,
        sai_object_list_t,
        std::vector<sai_object_id_t>>;
  };
  using AttributeType = boost::variant<Attributes::MemberList>;

  struct MemberAttributes {
    using EnumType = sai_lag_member_attr_t;
    using LagId = SaiAttribute<
        EnumType,
        SAI_LAG_MEMBER_ATTR_LAG_ID,
        sai_object_id_t,
        SaiObjectIdT>;
    using PortId = SaiAttribute<
        EnumType,
        SAI_LAG_MEMBER_ATTR_PORT_ID,
        sai_object_id_t,
        SaiObjectIdT>;
  };
  using MemberAttributeType =
      boost::variant<MemberAttributes::LagId, MemberAttributes::PortId>;
  struct EntryType {};
};

class LagApi : public SaiApi<LagApi, LagTypes> {
 public:
  LagApi() {
    sai_status_t res = sai_api_query(
      SAI_API_LAG, reinterpret_cast<void**>(&api_));
    if (res != SAI_STATUS_SUCCESS) {
      throw SaiApiError(res);
    }
  }
  LagApi(const LagApi& other) = delete;
 private:
  sai_status_t _create(
      sai_object_id_t* lag_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_lag(lag_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t lag_id) {
    return api_->remove_lag(lag_id);
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t handle) const {
    return api_->get_lag_attribute(handle, 1, attr);
  }
  sai_status_t _setAttr(const sai_attribute_t* attr, sai_object_id_t handle) {
    return api_->set_lag_attribute(handle, attr);
  }
  sai_status_t _createMember(
      sai_object_id_t* lag_member_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_lag_member(
        lag_member_id, switch_id, count, attr_list);
  }
  sai_status_t _removeMember(sai_object_id_t lag_member_id) {
    return api_->remove_lag_member(lag_member_id);
  }
  sai_status_t _getMemberAttr(sai_attribute_t* attr, sai_object_id_t handle)
      const {
    return api_->get_lag_member_attribute(handle, 1, attr);
  }
  sai_status_t _setMemberAttr(
      const sai_attribute_t* attr,
      sai_object_id_t handle) {
    return api_->set_lag_member_attribute(handle, attr);
  }
  sai_lag_api_t* api_;
  friend class SaiApi<LagApi, LagTypes>;
};

} // namespace fboss
} // namespace facebook
