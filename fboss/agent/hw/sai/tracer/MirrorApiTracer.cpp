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
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_mirror_session(
    sai_object_id_t* mirror_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->mirrorApi_->create_mirror_session(
      mirror_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_mirror_session",
      mirror_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_MIRROR_SESSION,
      rv);
  return rv;
}

sai_status_t wrap_remove_mirror_session(sai_object_id_t mirror_id) {
  auto rv =
      SaiTracer::getInstance()->mirrorApi_->remove_mirror_session(mirror_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_mirror_session", mirror_id, SAI_OBJECT_TYPE_MIRROR_SESSION, rv);
  return rv;
}

sai_status_t wrap_set_mirror_session_attribute(
    sai_object_id_t mirror_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->mirrorApi_->set_mirror_session_attribute(
      mirror_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_mirror_session_attribute",
      mirror_id,
      attr,
      SAI_OBJECT_TYPE_MIRROR_SESSION,
      rv);
  return rv;
}

sai_status_t wrap_get_mirror_session_attribute(
    sai_object_id_t mirror_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->mirrorApi_->get_mirror_session_attribute(
      mirror_id, attr_count, attr_list);
}

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
      case SAI_MIRROR_SESSION_ATTR_ERSPAN_ENCAPSULATION_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_MIRROR_SESSION_ATTR_TC:
      case SAI_MIRROR_SESSION_ATTR_TOS:
      case SAI_MIRROR_SESSION_ATTR_TTL:
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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
