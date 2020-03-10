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

#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class RouterInterfaceApi;

struct SaiRouterInterfaceTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_ROUTER_INTERFACE;
  using SaiApiT = RouterInterfaceApi;
  struct Attributes {
    using EnumType = sai_router_interface_attr_t;
    using SrcMac = SaiAttribute<
        EnumType,
        SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS,
        folly::MacAddress>;
    using Type =
        SaiAttribute<EnumType, SAI_ROUTER_INTERFACE_ATTR_TYPE, sai_int32_t>;
    using VirtualRouterId = SaiAttribute<
        EnumType,
        SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID,
        SaiObjectIdT>;
    using VlanId =
        SaiAttribute<EnumType, SAI_ROUTER_INTERFACE_ATTR_VLAN_ID, SaiObjectIdT>;
    using Mtu =
        SaiAttribute<EnumType, SAI_ROUTER_INTERFACE_ATTR_MTU, sai_uint32_t>;
  };
  using CreateAttributes = std::tuple<
      Attributes::VirtualRouterId,
      Attributes::Type,
      Attributes::VlanId,
      std::optional<Attributes::SrcMac>,
      std::optional<Attributes::Mtu>>;
  using AdapterKey = RouterInterfaceSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::VirtualRouterId, Attributes::VlanId>;
};

SAI_ATTRIBUTE_NAME(RouterInterface, SrcMac)
SAI_ATTRIBUTE_NAME(RouterInterface, Type)
SAI_ATTRIBUTE_NAME(RouterInterface, VirtualRouterId)
SAI_ATTRIBUTE_NAME(RouterInterface, VlanId)
SAI_ATTRIBUTE_NAME(RouterInterface, Mtu)

class RouterInterfaceApi : public SaiApi<RouterInterfaceApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_ROUTER_INTERFACE;
  RouterInterfaceApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(
        status, ApiType, "Failed to query for router interface api");
  }
  RouterInterfaceApi(const RouterInterfaceApi& other) = delete;

 private:
  sai_status_t _create(
      RouterInterfaceSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_router_interface(
        rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(RouterInterfaceSaiId router_interface_id) {
    return api_->remove_router_interface(router_interface_id);
  }
  sai_status_t _getAttribute(RouterInterfaceSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_router_interface_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(
      RouterInterfaceSaiId key,
      const sai_attribute_t* attr) {
    return api_->set_router_interface_attribute(key, attr);
  }

  sai_router_interface_api_t* api_;
  friend class SaiApi<RouterInterfaceApi>;
};

} // namespace facebook::fboss
