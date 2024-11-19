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
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/hash/Hash.h>
#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

bool operator==(
    const sai_system_port_config_t& lhs,
    const sai_system_port_config_t& rhs);
bool operator!=(
    const sai_system_port_config_t& lhs,
    const sai_system_port_config_t& rhs);
namespace facebook::fboss {

class SystemPortApi;

struct SaiSystemPortTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_SYSTEM_PORT;
  using SaiApiT = SystemPortApi;
  struct Attributes {
    using EnumType = sai_system_port_attr_t;
    using Type = SaiAttribute<EnumType, SAI_SYSTEM_PORT_ATTR_TYPE, sai_int32_t>;
    using QosNumberOfVoqs = SaiAttribute<
        EnumType,
        SAI_SYSTEM_PORT_ATTR_QOS_NUMBER_OF_VOQS,
        sai_uint32_t>;
    using QosVoqList = SaiAttribute<
        EnumType,
        SAI_SYSTEM_PORT_ATTR_QOS_VOQ_LIST,
        std::vector<sai_object_id_t>>;
    using Port =
        SaiAttribute<EnumType, SAI_SYSTEM_PORT_ATTR_PORT, SaiObjectIdT>;
    using AdminState = SaiAttribute<
        EnumType,
        SAI_SYSTEM_PORT_ATTR_ADMIN_STATE,
        bool,
        SaiBoolDefaultFalse>;
    using ConfigInfo = SaiAttribute<
        EnumType,
        SAI_SYSTEM_PORT_ATTR_CONFIG_INFO,
        sai_system_port_config_t>;
    using QosTcToQueueMap = SaiAttribute<
        EnumType,
        SAI_SYSTEM_PORT_ATTR_QOS_TC_TO_QUEUE_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    struct AttributeShelPktDstEnable {
      std::optional<sai_attr_id_t> operator()();
    };
    using ShelPktDstEnable = SaiExtensionAttribute<
        bool,
        AttributeShelPktDstEnable,
        SaiBoolDefaultFalse>;
  };
  using AdapterKey = SystemPortSaiId;
  using AdapterHostKey = Attributes::ConfigInfo;

  using CreateAttributes = std::tuple<
      Attributes::ConfigInfo,
      Attributes::AdminState,
      std::optional<Attributes::QosTcToQueueMap>,
      std::optional<Attributes::ShelPktDstEnable>>;
};

SAI_ATTRIBUTE_NAME(SystemPort, Type)
SAI_ATTRIBUTE_NAME(SystemPort, QosNumberOfVoqs)
SAI_ATTRIBUTE_NAME(SystemPort, QosVoqList)
SAI_ATTRIBUTE_NAME(SystemPort, Port)
SAI_ATTRIBUTE_NAME(SystemPort, AdminState)
SAI_ATTRIBUTE_NAME(SystemPort, ConfigInfo)
SAI_ATTRIBUTE_NAME(SystemPort, QosTcToQueueMap)
SAI_ATTRIBUTE_NAME(SystemPort, ShelPktDstEnable)

class SystemPortApi : public SaiApi<SystemPortApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_SYSTEM_PORT;
  SystemPortApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for port api");
  }

 private:
  sai_status_t _create(
      SystemPortSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_port(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(SystemPortSaiId key) const {
    return api_->remove_port(key);
  }
  sai_status_t _getAttribute(SystemPortSaiId key, sai_attribute_t* attr) const {
    return api_->get_port_attribute(key, 1, attr);
  }
  sai_status_t _setAttribute(SystemPortSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_port_attribute(key, attr);
  }
  sai_port_api_t* api_;
  friend class SaiApi<SystemPortApi>;
};
} // namespace facebook::fboss

namespace std {
template <>
struct hash<facebook::fboss::SaiSystemPortTraits::Attributes::ConfigInfo> {
  size_t operator()(
      const facebook::fboss::SaiSystemPortTraits::Attributes::ConfigInfo& key)
      const {
    const auto& val = key.value();
    return folly::hash::hash_combine(
        val.port_id,
        val.attached_switch_id,
        val.attached_core_index,
        val.attached_core_port_index,
        val.speed,
        val.num_voq);
  }
};

} // namespace std
