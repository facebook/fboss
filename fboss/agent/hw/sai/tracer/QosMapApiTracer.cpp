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
#include "fboss/agent/hw/sai/tracer/Utils.h"

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

void setQosMapAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_QOS_MAP_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST:
        qosMapListAttr(attr_list, i, listCount++, attrLines);
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
