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

sai_status_t wrap_create_samplepacket(
    sai_object_id_t* samplepacket_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->samplepacketApi_->create_samplepacket(
      samplepacket_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_samplepacket",
      samplepacket_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_SAMPLEPACKET,
      rv);
  return rv;
}

sai_status_t wrap_remove_samplepacket(sai_object_id_t samplepacket_id) {
  auto rv = SaiTracer::getInstance()->samplepacketApi_->remove_samplepacket(
      samplepacket_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_samplepacket", samplepacket_id, SAI_OBJECT_TYPE_SAMPLEPACKET, rv);
  return rv;
}

sai_status_t wrap_set_samplepacket_attribute(
    sai_object_id_t samplepacket_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->samplepacketApi_->set_samplepacket_attribute(
          samplepacket_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_samplepacket_attribute",
      samplepacket_id,
      attr,
      SAI_OBJECT_TYPE_SAMPLEPACKET,
      rv);
  return rv;
}

sai_status_t wrap_get_samplepacket_attribute(
    sai_object_id_t samplepacket_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->samplepacketApi_->get_samplepacket_attribute(
      samplepacket_id, attr_count, attr_list);
}

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
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
