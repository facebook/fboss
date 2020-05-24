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
namespace facebook::fboss {

class FakeAclEntry {
 public:
  explicit FakeAclEntry(sai_object_id_t tableId) : tableId(tableId) {}

  sai_object_id_t tableId;
  sai_uint32_t priority;
  sai_uint8_t fieldDscp;

  sai_object_id_t id;
};

class FakeAclTable {
 public:
  FakeAclTable(
      sai_int32_t stage,
      std::vector<sai_int32_t> bindPointTypeList,
      std::vector<sai_int32_t> actionTypeList)
      : stage(stage),
        bindPointTypeList(bindPointTypeList),
        actionTypeList(actionTypeList) {}

  static sai_acl_api_t* kApi();

  sai_object_id_t id;

  sai_int32_t stage;
  std::vector<sai_int32_t> bindPointTypeList;
  std::vector<sai_int32_t> actionTypeList;

  FakeManager<sai_object_id_t, FakeAclEntry>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeAclEntry>& fm() const {
    return fm_;
  }

 private:
  FakeManager<sai_object_id_t, FakeAclEntry> fm_;
};

using FakeAclTableManager = FakeManagerWithMembers<FakeAclTable, FakeAclEntry>;

void populate_acl_api(sai_acl_api_t** acl_api);

} // namespace facebook::fboss
