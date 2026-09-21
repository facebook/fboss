/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/QosMapApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/QosMapApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _QosMapMap{
    SAI_ATTR_MAP(QosMap, Type),
    SAI_ATTR_MAP(QosMap, MapToValueList),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(qos_map, SAI_OBJECT_TYPE_QOS_MAP, qosMap);
WRAP_REMOVE_FUNC(qos_map, SAI_OBJECT_TYPE_QOS_MAP, qosMap);
WRAP_SET_ATTR_FUNC(qos_map, SAI_OBJECT_TYPE_QOS_MAP, qosMap);
WRAP_GET_ATTR_FUNC(qos_map, SAI_OBJECT_TYPE_QOS_MAP, qosMap);

sai_qos_map_api_t* wrappedQosMapApi() {
  static sai_qos_map_api_t qosMapWrappers;

  qosMapWrappers.create_qos_map = &wrap_create_qos_map;
  qosMapWrappers.remove_qos_map = &wrap_remove_qos_map;
  qosMapWrappers.set_qos_map_attribute = &wrap_set_qos_map_attribute;
  qosMapWrappers.get_qos_map_attribute = &wrap_get_qos_map_attribute;

  return &qosMapWrappers;
}

SET_SAI_ATTRIBUTES(QosMap)

} // namespace facebook::fboss
