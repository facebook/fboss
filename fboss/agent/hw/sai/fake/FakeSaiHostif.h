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

sai_status_t send_hostif_packet_fn(
    sai_object_id_t switch_id,
    sai_size_t buffer_size,
    const void *buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

namespace facebook {
namespace fboss {

class FakeHostif {
 public:
  FakeHostif() {}
};

void populate_hostif_api(sai_hostif_api_t** hostif_api);
} // namespace fboss
} // namespace facebook
