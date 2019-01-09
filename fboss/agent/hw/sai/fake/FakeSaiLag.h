/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
  #include <sai.h>
}

sai_status_t create_lag_fn(
    sai_object_id_t* lag_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_lag_fn(sai_object_id_t lag_id);

sai_status_t get_lag_attribute_fn(
    sai_object_id_t lag_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

sai_status_t set_lag_attribute_fn(
    sai_object_id_t lag_id,
    const sai_attribute_t* attr);

sai_status_t create_lag_member_fn(
    sai_object_id_t* lag_member_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_lag_member_fn(sai_object_id_t lag_member_id);

sai_status_t get_lag_member_attribute_fn(
    sai_object_id_t lag_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

sai_status_t set_lag_member_attribute_fn(
    sai_object_id_t lag_member_id,
    const sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeLagMember {
 public:
  explicit FakeLagMember(sai_object_id_t lagId) :
    lagId(lagId) {}
  sai_object_id_t lagId;
  sai_object_id_t id;
  sai_object_id_t portId;
};

class FakeLag {
 public:
  sai_object_id_t id;
  FakeManager<sai_object_id_t, FakeLagMember>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeLagMember>& fm() const {
    return fm_;
  }

 private:
  FakeManager<sai_object_id_t, FakeLagMember> fm_;
};

using FakeLagManager = FakeManagerWithMembers<FakeLag, FakeLagMember>;

void populate_lag_api(sai_lag_api_t** lag_api);
} // namespace fboss
} // namespace facebook
