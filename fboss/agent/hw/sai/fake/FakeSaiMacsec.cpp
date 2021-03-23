/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/fake/FakeSaiPort.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeSai;

sai_status_t create_macsec_fn(
    sai_object_id_t* macsec_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_macsec_direction_t direction;
  std::optional<bool> physicalBypass;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_ATTR_DIRECTION:
        direction = static_cast<sai_macsec_direction_t>(attr_list[i].value.s32);
        break;
      case SAI_MACSEC_ATTR_PHYSICAL_BYPASS_ENABLE:
        physicalBypass = attr_list[i].value.booldata;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }

  *macsec_id = fs->macsecManager.create(direction);
  auto& macsec = fs->macsecManager.get(*macsec_id);
  if (physicalBypass) {
    macsec.physicalBypass = physicalBypass.value();
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_macsec_fn(sai_object_id_t macsec_id) {
  FakeSai::getInstance()->macsecManager.remove(macsec_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_macsec_attribute_fn(
    sai_object_id_t macsec_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& macsec = fs->macsecManager.get(macsec_id);
  sai_status_t res = SAI_STATUS_SUCCESS;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_MACSEC_ATTR_PHYSICAL_BYPASS_ENABLE:
      macsec.physicalBypass = attr->value.booldata;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t get_macsec_attribute_fn(
    sai_object_id_t macsec_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& macsec = fs->macsecManager.get(macsec_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_MACSEC_ATTR_DIRECTION:
        attr[i].value.s32 = macsec.direction;
        break;
      case SAI_MACSEC_ATTR_PHYSICAL_BYPASS_ENABLE:
        attr[i].value.booldata = macsec.physicalBypass;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_macsec_api_t _macsec_api;

void populate_macsec_api(sai_macsec_api_t** macsec_api) {
  _macsec_api.create_macsec = &create_macsec_fn;
  _macsec_api.remove_macsec = &remove_macsec_fn;
  _macsec_api.set_macsec_attribute = &set_macsec_attribute_fn;
  _macsec_api.get_macsec_attribute = &get_macsec_attribute_fn;

  // TODO(ccpowers): add stats apis (get/ext/clear)

  *macsec_api = &_macsec_api;
}

} // namespace facebook::fboss
