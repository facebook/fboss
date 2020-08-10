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

sai_status_t wrap_create_qos_map(
    sai_object_id_t* qos_map_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->qosMapApi_->create_qos_map(
      qos_map_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_qos_map",
      qos_map_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_QOS_MAP,
      rv);
  return rv;
}

sai_status_t wrap_remove_qos_map(sai_object_id_t qos_map_id) {
  auto rv = SaiTracer::getInstance()->qosMapApi_->remove_qos_map(qos_map_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_qos_map", qos_map_id, SAI_OBJECT_TYPE_QOS_MAP, rv);
  return rv;
}

sai_status_t wrap_set_qos_map_attribute(
    sai_object_id_t qos_map_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->qosMapApi_->set_qos_map_attribute(
      qos_map_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_qos_map_attribute", qos_map_id, attr, SAI_OBJECT_TYPE_QOS_MAP, rv);
  return rv;
}

sai_status_t wrap_get_qos_map_attribute(
    sai_object_id_t qos_map_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->qosMapApi_->get_qos_map_attribute(
      qos_map_id, attr_count, attr_list);
}

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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
