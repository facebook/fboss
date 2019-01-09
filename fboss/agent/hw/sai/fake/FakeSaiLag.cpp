/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "FakeSaiLag.h"
#include "FakeSai.h"

#include <folly/logging/xlog.h>
#include <folly/Optional.h>

using facebook::fboss::FakeLag;
using facebook::fboss::FakeLagMember;
using facebook::fboss::FakeSai;

sai_status_t create_lag_fn(
    sai_object_id_t* lag_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  *lag_id = fs->lm.create();
  for (int i = 0; i < attr_count; ++i) {
    sai_status_t res = set_lag_attribute_fn(*lag_id, &attr_list[i]);
    if (res != SAI_STATUS_SUCCESS) {
      fs->lm.remove(*lag_id);
      return res;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_lag_fn(sai_object_id_t lag_id) {
  auto fs = FakeSai::getInstance();
  fs->lm.remove(lag_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_lag_attribute_fn(
    sai_object_id_t lag_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_LAG_ATTR_PORT_LIST:
        {
        const auto& lagMemberMap = fs->lm.get(lag_id).fm().map();
        attr[i].value.objlist.count = lagMemberMap.size();
        int j = 0;
        for (const auto& m : lagMemberMap) {
          attr[i].value.objlist.list[j++] = m.first;
        }
        }
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_lag_attribute_fn(
    sai_object_id_t /* lag_id */,
    const sai_attribute_t* attr) {
  switch (attr->id) {
    default:
      return SAI_STATUS_NOT_SUPPORTED;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_lag_member_fn(
    sai_object_id_t* lag_member_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  folly::Optional<sai_object_id_t> lagId;
  folly::Optional<sai_object_id_t> portId;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_LAG_MEMBER_ATTR_LAG_ID:
        lagId = attr_list[i].value.oid;
        break;
      case SAI_LAG_MEMBER_ATTR_PORT_ID:
        portId = attr_list[i].value.oid;
        break;
    }
  }

  if (!lagId || !portId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *lag_member_id = fs->lm.createMember(lagId.value(), portId.value());
  for (int i = 0; i < attr_count; ++i) {
    sai_status_t res =
        set_lag_member_attribute_fn(*lag_member_id, &attr_list[i]);
    if (res != SAI_STATUS_SUCCESS) {
      fs->lm.removeMember(*lag_member_id);
      return res;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_lag_member_fn(sai_object_id_t lag_member_id) {
  auto fs = FakeSai::getInstance();
  fs->lm.removeMember(lag_member_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_lag_member_attribute_fn(
    sai_object_id_t lag_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& lagMember = fs->lm.getMember(lag_member_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_LAG_MEMBER_ATTR_LAG_ID:
        attr[i].value.oid = lagMember.lagId;
        break;
      case SAI_LAG_MEMBER_ATTR_PORT_ID:
        attr[i].value.oid = lagMember.portId;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_lag_member_attribute_fn(
    sai_object_id_t lag_member_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& lagMember = fs->lm.getMember(lag_member_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_LAG_MEMBER_ATTR_LAG_ID:
      lagMember.lagId = attr->value.oid;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_LAG_MEMBER_ATTR_PORT_ID:
      lagMember.portId = attr->value.oid;
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_NOT_SUPPORTED;
      break;
  }
  return res;
}

namespace facebook {
namespace fboss {

static sai_lag_api_t _lag_api;

void populate_lag_api(sai_lag_api_t** lag_api) {
  _lag_api.create_lag = &create_lag_fn;
  _lag_api.remove_lag = &remove_lag_fn;
  _lag_api.set_lag_attribute = &set_lag_attribute_fn;
  _lag_api.get_lag_attribute = &get_lag_attribute_fn;
  _lag_api.create_lag_member = &create_lag_member_fn;
  _lag_api.remove_lag_member = &remove_lag_member_fn;
  _lag_api.set_lag_member_attribute = &set_lag_member_attribute_fn;
  _lag_api.get_lag_member_attribute = &get_lag_member_attribute_fn;
  *lag_api = &_lag_api;
}

} // namespace fboss
} // namespace facebook
