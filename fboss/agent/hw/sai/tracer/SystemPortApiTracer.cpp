/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SystemPortApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/SystemPortApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _SystemPortMap{
    SAI_ATTR_MAP(SystemPort, Type),
    SAI_ATTR_MAP(SystemPort, QosNumberOfVoqs),
    SAI_ATTR_MAP(SystemPort, QosVoqList),
    SAI_ATTR_MAP(SystemPort, Port),
    SAI_ATTR_MAP(SystemPort, AdminState),
    SAI_ATTR_MAP(SystemPort, ConfigInfo),
    SAI_ATTR_MAP(SystemPort, QosTcToQueueMap),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(system_port, SAI_OBJECT_TYPE_SYSTEM_PORT, systemPort);
WRAP_REMOVE_FUNC(system_port, SAI_OBJECT_TYPE_SYSTEM_PORT, systemPort);
WRAP_SET_ATTR_FUNC(system_port, SAI_OBJECT_TYPE_SYSTEM_PORT, systemPort);
WRAP_GET_ATTR_FUNC(system_port, SAI_OBJECT_TYPE_SYSTEM_PORT, systemPort);

sai_system_port_api_t* wrappedSystemPortApi() {
  static sai_system_port_api_t systemPortWrappers;

  systemPortWrappers.create_system_port = &wrap_create_system_port;
  systemPortWrappers.remove_system_port = &wrap_remove_system_port;
  systemPortWrappers.set_system_port_attribute =
      &wrap_set_system_port_attribute;
  systemPortWrappers.get_system_port_attribute =
      &wrap_get_system_port_attribute;
  return &systemPortWrappers;
}

SET_SAI_ATTRIBUTES(SystemPort)

} // namespace facebook::fboss
