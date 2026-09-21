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
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _InsegEntryMap{
    SAI_ATTR_MAP(InSeg, NextHopId),
    SAI_ATTR_MAP(InSeg, PacketAction),
    SAI_ATTR_MAP(InSeg, NumOfPop),
};
} // namespace

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

SET_SAI_ATTRIBUTES(InsegEntry)

} // namespace facebook::fboss
