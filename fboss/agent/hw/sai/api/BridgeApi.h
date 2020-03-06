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
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>
#include <variant>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class BridgeApi;

struct SaiBridgeTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_BRIDGE;
  using SaiApiT = BridgeApi;
  struct Attributes {
    using EnumType = sai_bridge_attr_t;
    using PortList = SaiAttribute<
        EnumType,
        SAI_BRIDGE_ATTR_PORT_LIST,
        std::vector<sai_object_id_t>>;
    using Type = SaiAttribute<EnumType, SAI_BRIDGE_ATTR_TYPE, sai_int32_t>;
  };
  using AdapterKey = BridgeSaiId;
  using AdapterHostKey = std::monostate;
  using CreateAttributes = std::tuple<Attributes::Type>;
};

SAI_ATTRIBUTE_NAME(Bridge, PortList)
SAI_ATTRIBUTE_NAME(Bridge, Type)

template <>
struct IsSaiObjectOwnedByAdapter<SaiBridgeTraits> : public std::true_type {};

struct SaiBridgePortTraits {
  static constexpr sai_api_t ApiType = SAI_API_BRIDGE;
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_BRIDGE_PORT;
  using SaiApiT = BridgeApi;
  struct Attributes {
    using EnumType = sai_bridge_port_attr_t;
    using BridgeId =
        SaiAttribute<EnumType, SAI_BRIDGE_PORT_ATTR_BRIDGE_ID, SaiObjectIdT>;
    using PortId =
        SaiAttribute<EnumType, SAI_BRIDGE_PORT_ATTR_PORT_ID, SaiObjectIdT>;
    using Type = SaiAttribute<EnumType, SAI_BRIDGE_PORT_ATTR_TYPE, sai_int32_t>;
    using FdbLearningMode = SaiAttribute<
        EnumType,
        SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE,
        sai_int32_t>;
    using AdminState =
        SaiAttribute<EnumType, SAI_BRIDGE_PORT_ATTR_ADMIN_STATE, bool>;
  };
  using AdapterKey = BridgePortSaiId;
  using AdapterHostKey = Attributes::PortId;
  using CreateAttributes = std::tuple<
      Attributes::Type,
      Attributes::PortId,
      Attributes::AdminState,
      std::optional<Attributes::FdbLearningMode>>;
};

SAI_ATTRIBUTE_NAME(BridgePort, BridgeId)
SAI_ATTRIBUTE_NAME(BridgePort, PortId)
SAI_ATTRIBUTE_NAME(BridgePort, Type)
SAI_ATTRIBUTE_NAME(BridgePort, FdbLearningMode)
SAI_ATTRIBUTE_NAME(BridgePort, AdminState)

class BridgeApi : public SaiApi<BridgeApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_BRIDGE;
  BridgeApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for bridge api");
  }

 private:
  sai_status_t _create(
      BridgeSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_bridge(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _create(
      BridgePortSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_bridge_port(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(BridgeSaiId id) {
    CHECK(false) << " Bridge remove should never be called";
    return SAI_STATUS_FAILURE;
  }
  sai_status_t _remove(BridgePortSaiId id) {
    return api_->remove_bridge_port(id);
  }

  sai_status_t _getAttribute(BridgeSaiId id, sai_attribute_t* attr) const {
    return api_->get_bridge_attribute(id, 1, attr);
  }
  sai_status_t _getAttribute(BridgePortSaiId id, sai_attribute_t* attr) const {
    return api_->get_bridge_port_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(BridgeSaiId id, const sai_attribute_t* attr) {
    return api_->set_bridge_attribute(id, attr);
  }
  sai_status_t _setAttribute(BridgePortSaiId id, const sai_attribute_t* attr) {
    return api_->set_bridge_port_attribute(id, attr);
  }

  sai_bridge_api_t* api_;
  friend class SaiApi<BridgeApi>;
};

} // namespace facebook::fboss
