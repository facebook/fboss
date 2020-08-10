/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/FdbApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_fdb_entry(
    const sai_fdb_entry_t* fdb_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->fdbApi_->create_fdb_entry(
      fdb_entry, attr_count, attr_list);

  SaiTracer::getInstance()->logFdbEntryCreateFn(
      fdb_entry, attr_count, attr_list, rv);
  return rv;
}

sai_status_t wrap_remove_fdb_entry(const sai_fdb_entry_t* fdb_entry) {
  auto rv = SaiTracer::getInstance()->fdbApi_->remove_fdb_entry(fdb_entry);

  SaiTracer::getInstance()->logFdbEntryRemoveFn(fdb_entry, rv);
  return rv;
}

sai_status_t wrap_set_fdb_entry_attribute(
    const sai_fdb_entry_t* fdb_entry,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->fdbApi_->set_fdb_entry_attribute(
      fdb_entry, attr);

  SaiTracer::getInstance()->logFdbEntrySetAttrFn(fdb_entry, attr, rv);
  return rv;
}

sai_status_t wrap_get_fdb_entry_attribute(
    const sai_fdb_entry_t* fdb_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->fdbApi_->get_fdb_entry_attribute(
      fdb_entry, attr_count, attr_list);
}

sai_fdb_api_t* wrappedFdbApi() {
  static sai_fdb_api_t fdbWrappers;

  fdbWrappers.create_fdb_entry = &wrap_create_fdb_entry;
  fdbWrappers.remove_fdb_entry = &wrap_remove_fdb_entry;
  fdbWrappers.set_fdb_entry_attribute = &wrap_set_fdb_entry_attribute;
  fdbWrappers.get_fdb_entry_attribute = &wrap_get_fdb_entry_attribute;

  return &fdbWrappers;
}

void setFdbEntryAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_FDB_ENTRY_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
