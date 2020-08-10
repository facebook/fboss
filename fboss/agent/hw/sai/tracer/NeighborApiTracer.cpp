/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/NeighborApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_neighbor_entry(
    const sai_neighbor_entry_t* neighbor_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->neighborApi_->create_neighbor_entry(
      neighbor_entry, attr_count, attr_list);

  SaiTracer::getInstance()->logNeighborEntryCreateFn(
      neighbor_entry, attr_count, attr_list, rv);
  return rv;
}

sai_status_t wrap_remove_neighbor_entry(
    const sai_neighbor_entry_t* neighbor_entry) {
  auto rv = SaiTracer::getInstance()->neighborApi_->remove_neighbor_entry(
      neighbor_entry);

  SaiTracer::getInstance()->logNeighborEntryRemoveFn(neighbor_entry, rv);
  return rv;
}

sai_status_t wrap_set_neighbor_entry_attribute(
    const sai_neighbor_entry_t* neighbor_entry,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->neighborApi_->set_neighbor_entry_attribute(
          neighbor_entry, attr);

  SaiTracer::getInstance()->logNeighborEntrySetAttrFn(neighbor_entry, attr, rv);
  return rv;
}

sai_status_t wrap_get_neighbor_entry_attribute(
    const sai_neighbor_entry_t* neighbor_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->neighborApi_->get_neighbor_entry_attribute(
      neighbor_entry, attr_count, attr_list);
}

sai_status_t wrap_remove_all_neighbor_entries(sai_object_id_t switch_id) {
  return SaiTracer::getInstance()->neighborApi_->remove_all_neighbor_entries(
      switch_id);
}

sai_neighbor_api_t* wrappedNeighborApi() {
  static sai_neighbor_api_t neighborWrappers;

  neighborWrappers.create_neighbor_entry = &wrap_create_neighbor_entry;
  neighborWrappers.remove_neighbor_entry = &wrap_remove_neighbor_entry;
  neighborWrappers.set_neighbor_entry_attribute =
      &wrap_set_neighbor_entry_attribute;
  neighborWrappers.get_neighbor_entry_attribute =
      &wrap_get_neighbor_entry_attribute;
  neighborWrappers.remove_all_neighbor_entries =
      &wrap_remove_all_neighbor_entries;

  return &neighborWrappers;
}

void setNeighborEntryAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS:
        macAddressAttr(attr_list, i, attrLines);
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
