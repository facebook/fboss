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

#include <folly/MacAddress.h>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class FakeVirtualRouter {
 public:
  FakeVirtualRouter() {}
  sai_object_id_t id;
  folly::MacAddress srcMac;
};

using FakeVirtualRouterManager =
    FakeManager<sai_object_id_t, FakeVirtualRouter>;

void populate_virtual_router_api(sai_virtual_router_api_t** virtual_router_api);

} // namespace facebook::fboss
