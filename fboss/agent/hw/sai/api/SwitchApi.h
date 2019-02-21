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
#include <folly/MacAddress.h>

#include <vector>

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

struct SwitchTypes {
  struct Attributes {
    using EnumType = sai_switch_attr_t;
    using DefaultVirtualRouterId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID,
        sai_object_id_t,
        SaiObjectIdT>;
    using DefaultVlanId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_DEFAULT_VLAN_ID,
        sai_object_id_t,
        SaiObjectIdT>;
    using PortNumber =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_PORT_NUMBER, sai_uint32_t>;
    using PortList = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_PORT_LIST,
        sai_object_list_t,
        std::vector<sai_object_id_t>>;
    using SrcMac = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SRC_MAC_ADDRESS,
        sai_mac_t,
        folly::MacAddress>;
  };
  using AttributeType = boost::variant<
      Attributes::DefaultVlanId,
      Attributes::PortNumber,
      Attributes::PortList,
      Attributes::SrcMac>;
  struct MemberAttributes {};
  using MemberAttributeType = boost::variant<boost::blank>;
  struct EntryType {};
};

class SwitchApi : public SaiApi<SwitchApi, SwitchTypes> {
 public:
    SwitchApi() {
      sai_status_t status =
          sai_api_query(SAI_API_SWITCH, reinterpret_cast<void**>(&api_));
    saiCheckError(status, "Failed to query for switch api");
  }
 private:
  sai_status_t _create(
      sai_object_id_t* switch_id,
      sai_attribute_t* attr_list,
      size_t attr_count) {
    return SAI_STATUS_FAILURE;
  }
  sai_status_t _remove(sai_object_id_t switch_id) {
    return SAI_STATUS_FAILURE;
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t switch_id)
      const {
    return api_->get_switch_attribute(switch_id, 1, attr);
  }
  sai_status_t _setAttr(
      const sai_attribute_t* attr,
      sai_object_id_t switch_id) {
    return api_->set_switch_attribute(switch_id, attr);
  }
  sai_switch_api_t* api_;
  friend class SaiApi<SwitchApi, SwitchTypes>;
};

} // namespace fboss
} // namespace facebook
