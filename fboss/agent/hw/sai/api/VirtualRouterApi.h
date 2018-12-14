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

struct VirtualRouterTypes {
  struct Attributes {
    using EnumType = sai_virtual_router_attr_t;
    using SrcMac = SaiAttribute<
        EnumType,
        SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS,
        sai_mac_t,
        folly::MacAddress>;
  };
  using AttributeType = boost::variant<Attributes::SrcMac>;
  struct MemberAttributes {};
  using MemberAttributeType = boost::variant<boost::blank>;
  struct EntryType {};
};

class VirtualRouterApi : public SaiApi<VirtualRouterApi, VirtualRouterTypes> {
 public:
  VirtualRouterApi() {
    sai_status_t res = sai_api_query(SAI_API_VIRTUAL_ROUTER, (void**)&api_);
    if (res != SAI_STATUS_SUCCESS) {
      throw SaiApiError(res);
    }
  }
  VirtualRouterApi(const VirtualRouterApi& other) = delete;
 private:
  sai_status_t _create(
      sai_object_id_t* virtual_router_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_virtual_router(
        virtual_router_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t virtual_router_id) {
    return api_->remove_virtual_router(virtual_router_id);
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t handle) const {
    return api_->get_virtual_router_attribute(handle, 1, attr);
  }
  sai_status_t _setAttr(const sai_attribute_t* attr, sai_object_id_t handle) {
    return api_->set_virtual_router_attribute(handle, attr);
  }
  sai_virtual_router_api_t* api_;
  friend class SaiApi<VirtualRouterApi, VirtualRouterTypes>;
};

} // namespace fboss
} // namespace facebook
