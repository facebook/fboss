/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/MirrorApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(mirror_session, SAI_OBJECT_TYPE_MIRROR_SESSION, mirror);
WRAP_REMOVE_FUNC(mirror_session, SAI_OBJECT_TYPE_MIRROR_SESSION, mirror);
WRAP_SET_ATTR_FUNC(mirror_session, SAI_OBJECT_TYPE_MIRROR_SESSION, mirror);
WRAP_GET_ATTR_FUNC(mirror_session, SAI_OBJECT_TYPE_MIRROR_SESSION, mirror);

sai_mirror_api_t* wrappedMirrorApi() {
  static sai_mirror_api_t mirrorWrappers;
  mirrorWrappers.create_mirror_session = &wrap_create_mirror_session;
  mirrorWrappers.remove_mirror_session = &wrap_remove_mirror_session;
  mirrorWrappers.set_mirror_session_attribute =
      &wrap_set_mirror_session_attribute;
  mirrorWrappers.get_mirror_session_attribute =
      &wrap_get_mirror_session_attribute;
  return &mirrorWrappers;
}

void setMirrorSessionAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MIRROR_SESSION_ATTR_MONITOR_PORT:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_MIRROR_SESSION_ATTR_TRUNCATE_SIZE:
      case SAI_MIRROR_SESSION_ATTR_GRE_PROTOCOL_TYPE:
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
      case SAI_MIRROR_SESSION_ATTR_UDP_SRC_PORT:
      case SAI_MIRROR_SESSION_ATTR_UDP_DST_PORT:
#endif
        attrLines.push_back(u16Attr(attr_list, i));
        break;
      case SAI_MIRROR_SESSION_ATTR_SAMPLE_RATE:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      case SAI_MIRROR_SESSION_ATTR_TYPE:
      case SAI_MIRROR_SESSION_ATTR_ERSPAN_ENCAPSULATION_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_MIRROR_SESSION_ATTR_TC:
      case SAI_MIRROR_SESSION_ATTR_TOS:
      case SAI_MIRROR_SESSION_ATTR_TTL:
      case SAI_MIRROR_SESSION_ATTR_IPHDR_VERSION:
        attrLines.push_back(u8Attr(attr_list, i));
        break;
      case SAI_MIRROR_SESSION_ATTR_SRC_MAC_ADDRESS:
      case SAI_MIRROR_SESSION_ATTR_DST_MAC_ADDRESS:
        macAddressAttr(attr_list, i, attrLines);
        break;
      case SAI_MIRROR_SESSION_ATTR_SRC_IP_ADDRESS:
      case SAI_MIRROR_SESSION_ATTR_DST_IP_ADDRESS:
        ipAttr(attr_list, i, attrLines);
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
