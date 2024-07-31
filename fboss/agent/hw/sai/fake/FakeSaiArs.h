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

struct FakeArs {
 public:
  FakeArs(sai_ars_mode_t mode, sai_uint32_t idle_time, sai_uint32_t max_flows)
      : mode(mode), idle_time(idle_time), max_flows(max_flows) {}
  sai_ars_mode_t mode;
  sai_uint32_t idle_time;
  sai_uint32_t max_flows;
  sai_object_id_t id;
};

// first next hop begins with 1
using FakeArsManager = FakeManager<sai_object_id_t, FakeArs, 1>;

void populate_ars_api(sai_ars_api_t** ars_api);

} // namespace facebook::fboss
