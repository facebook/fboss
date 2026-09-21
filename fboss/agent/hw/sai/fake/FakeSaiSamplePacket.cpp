/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/fake/FakeSaiPort.h"

#include <optional>

using facebook::fboss::FakePort;
using facebook::fboss::FakeSai;

sai_status_t create_samplepacket_fn(
    sai_object_id_t* samplepacket_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_samplepacket_type_t> type;
  std::optional<sai_samplepacket_mode_t> mode;
  std::optional<uint32_t> sampleRate;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_SAMPLEPACKET_ATTR_SAMPLE_RATE:
        sampleRate = attr_list[i].value.u32;
        break;
      case SAI_SAMPLEPACKET_ATTR_TYPE:
        type = static_cast<sai_samplepacket_type_t>(attr_list[i].value.s32);
        break;
      case SAI_SAMPLEPACKET_ATTR_MODE:
        mode = static_cast<sai_samplepacket_mode_t>(attr_list[i].value.s32);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }

  if (!type || !mode || !sampleRate) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *samplepacket_id = fs->samplePacketManager.create(
      sampleRate.value(), type.value(), mode.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_samplepacket_fn(sai_object_id_t samplepacket_id) {
  auto fs = FakeSai::getInstance();
  fs->samplePacketManager.remove(samplepacket_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_samplepacket_attribute_fn(
    sai_object_id_t samplepacket_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& samplePacket = fs->samplePacketManager.get(samplepacket_id);
  switch (attr->id) {
    case SAI_SAMPLEPACKET_ATTR_SAMPLE_RATE:
      samplePacket.sampleRate = attr->value.u32;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_samplepacket_attribute_fn(
    sai_object_id_t samplepacket_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  const auto& samplePacket = fs->samplePacketManager.get(samplepacket_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_SAMPLEPACKET_ATTR_SAMPLE_RATE:
        attr_list[i].value.u32 = samplePacket.sampleRate;
        break;
      case SAI_SAMPLEPACKET_ATTR_TYPE:
        attr_list[i].value.s32 = static_cast<int32_t>(samplePacket.type);
        break;
      case SAI_SAMPLEPACKET_ATTR_MODE:
        attr_list[i].value.s32 = static_cast<int32_t>(samplePacket.mode);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_samplepacket_api_t _samplepacket_api;

void populate_samplepacket_api(sai_samplepacket_api_t** samplepacket_api) {
  _samplepacket_api.create_samplepacket = &create_samplepacket_fn;
  _samplepacket_api.remove_samplepacket = &remove_samplepacket_fn;
  _samplepacket_api.set_samplepacket_attribute = &set_samplepacket_attribute_fn;
  _samplepacket_api.get_samplepacket_attribute = &get_samplepacket_attribute_fn;
  *samplepacket_api = &_samplepacket_api;
}

} // namespace facebook::fboss
