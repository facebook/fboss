/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/WredApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(wred, SAI_OBJECT_TYPE_WRED, wred);
WRAP_REMOVE_FUNC(wred, SAI_OBJECT_TYPE_WRED, wred);
WRAP_SET_ATTR_FUNC(wred, SAI_OBJECT_TYPE_WRED, wred);
WRAP_GET_ATTR_FUNC(wred, SAI_OBJECT_TYPE_WRED, wred);

sai_wred_api_t* wrappedWredApi() {
  static sai_wred_api_t WredWrappers;

  WredWrappers.create_wred = &wrap_create_wred;
  WredWrappers.remove_wred = &wrap_remove_wred;
  WredWrappers.set_wred_attribute = &wrap_set_wred_attribute;
  WredWrappers.get_wred_attribute = &wrap_get_wred_attribute;

  return &WredWrappers;
}

void setWredAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_WRED_ATTR_GREEN_ENABLE:
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      case SAI_WRED_ATTR_ECN_MARK_MODE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_WRED_ATTR_GREEN_MIN_THRESHOLD:
      case SAI_WRED_ATTR_GREEN_MAX_THRESHOLD:
      case SAI_WRED_ATTR_GREEN_DROP_PROBABILITY:
      case SAI_WRED_ATTR_ECN_GREEN_MIN_THRESHOLD:
      case SAI_WRED_ATTR_ECN_GREEN_MAX_THRESHOLD:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
