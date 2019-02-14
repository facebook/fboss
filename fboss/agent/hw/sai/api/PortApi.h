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

#include <folly/logging/xlog.h>

#include <boost/variant.hpp>

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

struct PortApiParameters {
  struct Attributes {
    using EnumType = sai_port_attr_t;
    using AdminState = SaiAttribute<EnumType, SAI_PORT_ATTR_ADMIN_STATE, bool>;
    using HwLaneList = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_HW_LANE_LIST,
        sai_u32_list_t,
        std::vector<uint32_t>>;
    using Speed = SaiAttribute<EnumType, SAI_PORT_ATTR_SPEED, sai_uint32_t>;
    using Type = SaiAttribute<EnumType, SAI_PORT_ATTR_TYPE, sai_int32_t>;

    using CreateAttributes =
        SaiAttributeTuple<HwLaneList, Speed, SaiAttributeOptional<AdminState>>;

    Attributes(const CreateAttributes& attrs) {
      std::tie(hwLaneList, speed, adminState) = attrs.value();
    }

    CreateAttributes attrs() const {
      return {hwLaneList, speed, adminState};
    }
    bool operator==(const Attributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const Attributes& other) const {
      return !(*this == other);
    }
    typename HwLaneList::ValueType hwLaneList;
    typename Speed::ValueType speed;
    folly::Optional<typename AdminState::ValueType> adminState;
  };
  using AttributeType = boost::variant<
      Attributes::AdminState,
      Attributes::HwLaneList,
      Attributes::Speed,
      Attributes::Type>;
  struct MemberAttributes {};
  using MemberAttributeType = boost::variant<boost::blank>;
  struct EntryType {};
};

class PortApi : public SaiApi<PortApi, PortApiParameters> {
 public:
  PortApi() {
    sai_status_t status =
        sai_api_query(SAI_API_PORT, reinterpret_cast<void**>(&api_));
    saiCheckError(status, "Failed to query for port api");
  }
 private:
  sai_status_t _create(
      sai_object_id_t* port_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_port(port_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t port_id) {
    return api_->remove_port(port_id);
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t id) const {
    return api_->get_port_attribute(id, 1, attr);
  }
  sai_status_t _setAttr(const sai_attribute_t* attr, sai_object_id_t id) {
    return api_->set_port_attribute(id, attr);
  }
  sai_port_api_t* api_;
  friend class SaiApi<PortApi, PortApiParameters>;
};

} // namespace fboss
} // namespace facebook
