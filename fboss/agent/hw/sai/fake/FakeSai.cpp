/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FakeSai.h"

#include <folly/Singleton.h>

#include <folly/logging/xlog.h>

namespace {
  struct singleton_tag_type {};
}

using facebook::fboss::FakeSai;
static folly::Singleton<FakeSai, singleton_tag_type> fakeSaiSingleton{};
std::shared_ptr<FakeSai> FakeSai::getInstance() {
  return fakeSaiSingleton.try_get();
}

sai_status_t sai_api_initialize(
    uint64_t /* flags */,
    const sai_service_method_table_t* /* services */) {
  auto fs = FakeSai::getInstance();
  if (fs->initialized) {
    return SAI_STATUS_FAILURE;
  }
  fs->initialized = true;
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_api_query(sai_api_t sai_api_id, void** api_method_table) {
  auto fs = FakeSai::getInstance();
  if (!fs->initialized) {
    return SAI_STATUS_FAILURE;
  }
  sai_status_t res;
  switch (sai_api_id) {
    case SAI_API_SWITCH:
      facebook::fboss::populate_switch_api(
          (sai_switch_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_PORT:
      facebook::fboss::populate_port_api((sai_port_api_t **)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}
