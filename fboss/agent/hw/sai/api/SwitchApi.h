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
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class SwitchApi;

struct SaiSwitchTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_SWITCH;
  using SaiApiT = SwitchApi;
  struct Attributes {
    using EnumType = sai_switch_attr_t;
    using CpuPort =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_CPU_PORT, SaiObjectIdT>;
    using DefaultVirtualRouterId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID,
        SaiObjectIdT>;
    using DefaultVlanId =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_DEFAULT_VLAN_ID, SaiObjectIdT>;
    using PortNumber =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_PORT_NUMBER, sai_uint32_t>;
    using PortList = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_PORT_LIST,
        std::vector<sai_object_id_t>>;
    using SrcMac = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SRC_MAC_ADDRESS,
        folly::MacAddress>;
    using HwInfo = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SWITCH_HARDWARE_INFO,
        std::vector<int8_t>>;
    using Default1QBridgeId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID,
        SaiObjectIdT>;
    using InitSwitch =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_INIT_SWITCH, bool>;
    using MaxNumberOfSupportedPorts = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_MAX_NUMBER_OF_SUPPORTED_PORTS,
        sai_uint32_t>;
    using SwitchShellEnable =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_SWITCH_SHELL_ENABLE, bool>;
    using NumberOfUnicastQueues = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_UNICAST_QUEUES,
        sai_uint32_t>;
    using NumberOfMulticastQueues = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_MULTICAST_QUEUES,
        sai_uint32_t>;
    using NumberOfQueues =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_NUMBER_OF_QUEUES, sai_uint32_t>;
    using NumberOfCpuQueues = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_CPU_QUEUES,
        sai_uint32_t>;
    using EcmpHash =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_ECMP_HASH, SaiObjectIdT>;
    using LagHash =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_LAG_HASH, SaiObjectIdT>;
  };
  using AdapterKey = SwitchSaiId;
  using AdapterHostKey = std::monostate;
  using CreateAttributes = std::tuple<
      Attributes::InitSwitch,
      std::optional<Attributes::HwInfo>,
      std::optional<Attributes::SrcMac>,
      std::optional<Attributes::SwitchShellEnable>>;
};

class SwitchApi : public SaiApi<SwitchApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_SWITCH;
  SwitchApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for switch api");
  }
  const sai_switch_api_t* api() const {
    return api_;
  }
  sai_switch_api_t* api() {
    return api_;
  }

  void registerRxCallback(
      SwitchSaiId id,
      sai_packet_event_notification_fn rx_cb);
  void registerPortStateChangeCallback(
      SwitchSaiId id,
      sai_port_state_change_notification_fn port_state_change_cb);
  void registerFdbEventCallback(
      SwitchSaiId id,
      sai_fdb_event_notification_fn fdb_event_cb);

  void unregisterRxCallback(SwitchSaiId switch_id) {
    registerRxCallback(switch_id, nullptr);
  }
  void unregisterPortStateChangeCallback(SwitchSaiId id) {
    registerPortStateChangeCallback(id, nullptr);
  }
  void unregisterFdbEventCallback(SwitchSaiId id) {
    registerFdbEventCallback(id, nullptr);
  }

 private:
  sai_status_t _create(
      SwitchSaiId* id,
      sai_object_id_t /* switch_id */,
      size_t attr_count,
      sai_attribute_t* attr_list) {
    return api_->create_switch(rawSaiId(id), attr_count, attr_list);
  }
  sai_status_t _remove(SwitchSaiId id) {
    return api_->remove_switch(id);
  }
  sai_status_t _getAttribute(SwitchSaiId id, sai_attribute_t* attr) const {
    return api_->get_switch_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(SwitchSaiId id, const sai_attribute_t* attr) {
    return api_->set_switch_attribute(id, attr);
  }

  sai_switch_api_t* api_;
  friend class SaiApi<SwitchApi>;
};

} // namespace facebook::fboss
