/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SamplePacketApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(samplepacket, SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket);
WRAP_REMOVE_FUNC(samplepacket, SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket);
WRAP_SET_ATTR_FUNC(samplepacket, SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket);
WRAP_GET_ATTR_FUNC(samplepacket, SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket);

sai_samplepacket_api_t* wrappedSamplePacketApi() {
  static sai_samplepacket_api_t samplepacketWrappers;
  samplepacketWrappers.create_samplepacket = &wrap_create_samplepacket;
  samplepacketWrappers.remove_samplepacket = &wrap_remove_samplepacket;
  samplepacketWrappers.set_samplepacket_attribute =
      &wrap_set_samplepacket_attribute;
  samplepacketWrappers.get_samplepacket_attribute =
      &wrap_get_samplepacket_attribute;
  return &samplepacketWrappers;
}

void setSamplePacketAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_SAMPLEPACKET_ATTR_SAMPLE_RATE:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      case SAI_SAMPLEPACKET_ATTR_TYPE:
      case SAI_SAMPLEPACKET_ATTR_MODE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
