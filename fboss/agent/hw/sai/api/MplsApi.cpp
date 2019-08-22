// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/MplsApi.h"

namespace facebook {
namespace fboss {

sai_status_t MplsApi::_create(
    const MplsApi::InSegEntry& inSegEntry,
    sai_attribute_t* attr_list,
    size_t attr_count) {
  // TODO(pshaikh) : add log
  return api_->create_inseg_entry(inSegEntry.entry(), attr_count, attr_list);
}
sai_status_t MplsApi::_remove(const MplsApi::InSegEntry& inSegEntry) {
  // TODO(pshaikh) : add log
  return api_->remove_inseg_entry(inSegEntry.entry());
}
sai_status_t MplsApi::_getAttr(
    sai_attribute_t* attr,
    const MplsApi::InSegEntry& inSegEntry) const {
  // TODO(pshaikh) : add log
  return api_->get_inseg_entry_attribute(inSegEntry.entry(), 1, attr);
}

sai_status_t MplsApi::_setAttr(
    const sai_attribute_t* attr,
    const MplsApi::InSegEntry& inSegEntry) {
  // TODO(pshaikh) : add log
  return api_->set_inseg_entry_attribute(inSegEntry.entry(), attr);
}

} // namespace fboss
} // namespace facebook
