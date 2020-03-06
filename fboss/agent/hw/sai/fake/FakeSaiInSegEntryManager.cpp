// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/fake/FakeSaiInSegEntryManager.h"

#include "fboss/agent/hw/sai/fake/FakeSai.h"

namespace {
using facebook::fboss::FakeSai;
using facebook::fboss::FakeSaiInSegEntry;

sai_status_t sai_remove_inseg_entry(const sai_inseg_entry_t* inseg_entry) {
  auto fakesai = FakeSai::getInstance();
  if (fakesai->inSegEntryManager.remove(FakeSaiInSegEntry(*inseg_entry)) == 0) {
    return SAI_STATUS_FAILURE;
  };
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_set_inseg_entry_attribute(
    const sai_inseg_entry_t* inseg_entry,
    const sai_attribute_t* attr) {
  auto fakesai = FakeSai::getInstance();
  auto& entry = fakesai->inSegEntryManager.get(FakeSaiInSegEntry(*inseg_entry));
  switch (attr->id) {
    case SAI_INSEG_ENTRY_ATTR_NEXT_HOP_ID:
      entry.nexthop = attr->value.oid;
      break;
    case SAI_INSEG_ENTRY_ATTR_NUM_OF_POP:
      entry.popcount = attr->value.u8;
      break;
    case SAI_INSEG_ENTRY_ATTR_PACKET_ACTION:
      entry.action = static_cast<sai_packet_action_t>(attr->value.s32);
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_get_inseg_entry_attribute(
    const sai_inseg_entry_t* inseg_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fakesai = FakeSai::getInstance();
  auto& entry = fakesai->inSegEntryManager.get(FakeSaiInSegEntry(*inseg_entry));
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_INSEG_ENTRY_ATTR_NEXT_HOP_ID:
        attr_list[i].value.oid = entry.nexthop;
        break;
      case SAI_INSEG_ENTRY_ATTR_NUM_OF_POP:
        attr_list[i].value.u8 = entry.popcount;
        break;
      case SAI_INSEG_ENTRY_ATTR_PACKET_ACTION:
        attr_list[i].value.s32 = entry.action;
        break;
      default:
        // TODO(pshaikh): warn here
        break;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_create_inseg_entry(
    const sai_inseg_entry_t* inseg_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fakesai = FakeSai::getInstance();
  fakesai->inSegEntryManager.create(FakeSaiInSegEntry(*inseg_entry));
  for (auto i = 0; i < attr_count; i++) {
    sai_set_inseg_entry_attribute(inseg_entry, &attr_list[i]);
  }
  return SAI_STATUS_SUCCESS;
}

} // namespace

namespace facebook::fboss {

sai_mpls_api_t* FakeInSegEntryManager::kApi() {
  static sai_mpls_api_t kMplsApi{&sai_create_inseg_entry,
                                 &sai_remove_inseg_entry,
                                 &sai_set_inseg_entry_attribute,
                                 &sai_get_inseg_entry_attribute};
  return &kMplsApi;
}

void populate_mpls_api(sai_mpls_api_t** mpls_api) {
  *mpls_api = FakeInSegEntryManager::kApi();
}

} // namespace facebook::fboss
