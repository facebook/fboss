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
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>
#include <vector>

extern "C" {
#include <sai.h>
}

FBOSS_STRONG_TYPE(sai_object_id_t, SwitchSaiId);

namespace facebook {
namespace fboss {

struct SaiSwitchTraits {
  static constexpr sai_api_t ApiType = SAI_API_SWITCH;
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_SWITCH;
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
  };
  using AdapterKey = SwitchSaiId;
  using AdapterHostKey = std::monostate;
  using CreateAttributes = std::tuple<
      Attributes::InitSwitch,
      std::optional<Attributes::HwInfo>,
      std::optional<Attributes::SrcMac>>;
  using AllAttributes = std::tuple<Attributes::InitSwitch>;
};

struct SwitchApiParameters {
  static constexpr sai_api_t ApiType = SAI_API_SWITCH;
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
    using NumberOfQueues =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_NUMBER_OF_QUEUES, sai_uint32_t>;
    using NumberOfCpuQueues = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_CPU_QUEUES,
        sai_uint32_t>;
    using CreateAttributes = SaiAttributeTuple<
        InitSwitch,
        SaiAttributeOptional<HwInfo>,
        SaiAttributeOptional<SrcMac>>;
    Attributes() {}
    Attributes(const CreateAttributes& attrs) {
      std::tie(initSwitch, hwInfo, srcMac) = attrs.value();
    }
    CreateAttributes attrs() const {
      return {initSwitch, hwInfo, srcMac};
    }
    bool operator==(const Attributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const Attributes& other) const {
      return !(*this == other);
    }
    typename InitSwitch::ValueType initSwitch;
    typename folly::Optional<HwInfo::ValueType> hwInfo;
    typename folly::Optional<SrcMac::ValueType> srcMac;
  };
};

class SwitchApi : public SaiApi<SwitchApi, SwitchApiParameters> {
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

  sai_status_t registerRxCallback(
      sai_object_id_t switch_id,
      sai_packet_event_notification_fn rx_cb) {
    sai_attribute_t attr;
    attr.id = SAI_SWITCH_ATTR_PACKET_EVENT_NOTIFY;
    attr.value.ptr = (void*)rx_cb;
    return _setAttr(&attr, switch_id);
  }

  sai_status_t registerPortStateChangeCallback(
      sai_object_id_t switch_id,
      sai_port_state_change_notification_fn port_state_change_cb) {
    sai_attribute_t attr;
    attr.id = SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY;
    attr.value.ptr = (void*)port_state_change_cb;
    return _setAttr(&attr, switch_id);
  }

 private:
  sai_status_t _create(
      sai_object_id_t* switch_id,
      sai_attribute_t* attr_list,
      size_t attr_count) {
    return api_->create_switch(switch_id, attr_count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t switch_id) {
    return api_->remove_switch(switch_id);
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
  friend class SaiApi<SwitchApi, SwitchApiParameters>;
};

} // namespace fboss
} // namespace facebook
