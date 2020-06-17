/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/fake/FakeSaiQosMap.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeQosMap;
using facebook::fboss::FakeSai;

sai_status_t create_qos_map_fn(
    sai_object_id_t* qos_map_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<int32_t> type;
  std::vector<sai_qos_map_t> mapToValueList;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_QOS_MAP_ATTR_TYPE:
        type = attr_list[i].value.s32;
        break;
      case SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST: {
        mapToValueList.reserve(attr_list[i].value.qosmap.count);
        for (int j = 0; j < attr_list[i].value.qosmap.count; ++j) {
          mapToValueList.push_back(attr_list[i].value.qosmap.list[j]);
        }
      } break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (type) {
    *qos_map_id =
        fs->qosMapManager.create(FakeQosMap(type.value(), mapToValueList));
  } else {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_qos_map_fn(sai_object_id_t qos_map_id) {
  auto fs = FakeSai::getInstance();
  fs->qosMapManager.remove(qos_map_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_qos_map_attribute_fn(
    sai_object_id_t qos_map_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& qm = fs->qosMapManager.get(qos_map_id);
  switch (attr->id) {
    case SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST: {
      std::vector<sai_qos_map_t> mapToValueList;
      mapToValueList.reserve(attr->value.qosmap.count);
      for (int i = 0; i < attr->value.qosmap.count; ++i) {
        mapToValueList.push_back(attr->value.qosmap.list[i]);
      }
      qm.mapToValueList = mapToValueList;
    } break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_qos_map_attribute_fn(
    sai_object_id_t qos_map_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& qm = fs->qosMapManager.get(qos_map_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_QOS_MAP_ATTR_TYPE:
        attr[i].value.s32 = qm.type;
        break;
      case SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST: {
        if (qm.mapToValueList.size() > attr[i].value.qosmap.count) {
          attr[i].value.qosmap.count = qm.mapToValueList.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.qosmap.count = qm.mapToValueList.size();
        int j = 0;
        for (const auto mapping : qm.mapToValueList) {
          attr[i].value.qosmap.list[j++] = mapping;
        }
        break;
      }
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_qos_map_api_t _qos_map_api;

void populate_qos_map_api(sai_qos_map_api_t** qos_map_api) {
  _qos_map_api.create_qos_map = &create_qos_map_fn;
  _qos_map_api.remove_qos_map = &remove_qos_map_fn;
  _qos_map_api.set_qos_map_attribute = &set_qos_map_attribute_fn;
  _qos_map_api.get_qos_map_attribute = &get_qos_map_attribute_fn;
  *qos_map_api = &_qos_map_api;
}

} // namespace facebook::fboss
