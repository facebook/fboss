/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/MplsApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_inseg_entry(
    const sai_inseg_entry_t* inseg_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->mplsApi_->create_inseg_entry(
      inseg_entry, attr_count, attr_list);

  SaiTracer::getInstance()->logInsegEntryCreateFn(
      inseg_entry, attr_count, attr_list, rv);
  return rv;
}

sai_status_t wrap_remove_inseg_entry(const sai_inseg_entry_t* inseg_entry) {
  auto rv = SaiTracer::getInstance()->mplsApi_->remove_inseg_entry(inseg_entry);

  SaiTracer::getInstance()->logInsegEntryRemoveFn(inseg_entry, rv);
  return rv;
}

sai_status_t wrap_set_inseg_entry_attribute(
    const sai_inseg_entry_t* inseg_entry,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->mplsApi_->set_inseg_entry_attribute(
      inseg_entry, attr);

  SaiTracer::getInstance()->logInsegEntrySetAttrFn(inseg_entry, attr, rv);
  return rv;
}

sai_status_t wrap_get_inseg_entry_attribute(
    const sai_inseg_entry_t* inseg_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->mplsApi_->get_inseg_entry_attribute(
      inseg_entry, attr_count, attr_list);
}

sai_mpls_api_t* wrappedMplsApi() {
  static sai_mpls_api_t mplsWrappers;

  mplsWrappers.create_inseg_entry = &wrap_create_inseg_entry;
  mplsWrappers.remove_inseg_entry = &wrap_remove_inseg_entry;
  mplsWrappers.set_inseg_entry_attribute = &wrap_set_inseg_entry_attribute;
  mplsWrappers.get_inseg_entry_attribute = &wrap_get_inseg_entry_attribute;

  return &mplsWrappers;
}

void setInsegEntryAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_INSEG_ENTRY_ATTR_NEXT_HOP_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_INSEG_ENTRY_ATTR_PACKET_ACTION:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_INSEG_ENTRY_ATTR_NUM_OF_POP:
        attrLines.push_back(u8Attr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
