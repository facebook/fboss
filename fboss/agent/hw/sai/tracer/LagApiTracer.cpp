/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/LagApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(lag, SAI_OBJECT_TYPE_LAG, lag);
WRAP_REMOVE_FUNC(lag, SAI_OBJECT_TYPE_LAG, lag);
WRAP_SET_ATTR_FUNC(lag, SAI_OBJECT_TYPE_LAG, lag);
WRAP_GET_ATTR_FUNC(lag, SAI_OBJECT_TYPE_LAG, lag);

WRAP_CREATE_FUNC(lag_member, SAI_OBJECT_TYPE_LAG_MEMBER, lag);
WRAP_REMOVE_FUNC(lag_member, SAI_OBJECT_TYPE_LAG_MEMBER, lag);
WRAP_SET_ATTR_FUNC(lag_member, SAI_OBJECT_TYPE_LAG_MEMBER, lag);
WRAP_GET_ATTR_FUNC(lag_member, SAI_OBJECT_TYPE_LAG_MEMBER, lag);

sai_lag_api_t* wrappedLagApi() {
  static sai_lag_api_t lagWrappers;

  lagWrappers.create_lag = &wrap_create_lag;
  lagWrappers.remove_lag = &wrap_remove_lag;
  lagWrappers.set_lag_attribute = &wrap_set_lag_attribute;
  lagWrappers.get_lag_attribute = &wrap_get_lag_attribute;
  lagWrappers.create_lag_member = &wrap_create_lag_member;
  lagWrappers.remove_lag_member = &wrap_remove_lag_member;
  lagWrappers.set_lag_member_attribute = &wrap_set_lag_member_attribute;
  lagWrappers.get_lag_member_attribute = &wrap_get_lag_member_attribute;

  return &lagWrappers;
}

void setLagAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      // Attribute(s) with SAI type sai_object_list_t
      case SAI_LAG_ATTR_PORT_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;

      // Attribute(s) with SAI type sai_object_id_t
      case SAI_LAG_ATTR_INGRESS_ACL:
      case SAI_LAG_ATTR_EGRESS_ACL:
        attrLines.push_back(oidAttr(attr_list, i));
        break;

      // Attribute(s) with SAI type sai_uint16_t
      case SAI_LAG_ATTR_PORT_VLAN_ID:
      case SAI_LAG_ATTR_TPID:
        attrLines.push_back(u16Attr(attr_list, i));
        break;

      // Attribute(s) with SAI type sai_uint8_t
      case SAI_LAG_ATTR_DEFAULT_VLAN_PRIORITY:
        attrLines.push_back(u8Attr(attr_list, i));
        break;

      // Attribute(s) with SAI type bool
      case SAI_LAG_ATTR_DROP_UNTAGGED:
      case SAI_LAG_ATTR_DROP_TAGGED:
        attrLines.push_back(boolAttr(attr_list, i));
        break;

      // Attribute(s) with SAI type sai_uint32_t
      case SAI_LAG_ATTR_SYSTEM_PORT_AGGREGATE_ID:
        attrLines.push_back(u32Attr(attr_list, i));
        break;

#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
        // Attribute(s) with SAI type char[32]
#ifdef IS_OSS
      case SAI_LAG_ATTR_END:
#else
      case SAI_LAG_ATTR_LABEL:
#endif
        charDataAttr(attr_list, i, attrLines);
        break;
#endif

      default:
        break;
    }
  }
}

void setLagMemberAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      // Attribute(s) with SAI type sai_object_id_t
      case SAI_LAG_MEMBER_ATTR_LAG_ID:
      case SAI_LAG_MEMBER_ATTR_PORT_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;

      // Attribute(s) with SAI type bool
      case SAI_LAG_MEMBER_ATTR_EGRESS_DISABLE:
      case SAI_LAG_MEMBER_ATTR_INGRESS_DISABLE:
        attrLines.push_back(boolAttr(attr_list, i));
        break;

      default:
        break;
    }
  }
}

} // namespace facebook::fboss
