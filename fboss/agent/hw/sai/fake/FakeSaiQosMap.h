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

struct FakeQosMap {
 public:
  FakeQosMap(int32_t type, const std::vector<sai_qos_map_t>& mapToValueList)
      : type(type), mapToValueList(mapToValueList) {}
  int32_t type;
  std::vector<sai_qos_map_t> mapToValueList;
  sai_object_id_t id;
};

using FakeQosMapManager = FakeManager<sai_object_id_t, FakeQosMap>;

void populate_qos_map_api(sai_qos_map_api_t** qos_map_api);

} // namespace facebook::fboss
