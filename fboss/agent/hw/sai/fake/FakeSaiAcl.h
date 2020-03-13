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

class FakeAcl {
 public:
  static sai_acl_api_t* kApi();

 private:
  sai_object_id_t id;
};

using FakeAclManager = FakeManager<sai_object_id_t, FakeAcl>;

void populate_acl_api(sai_acl_api_t** acl_api);

} // namespace facebook::fboss
