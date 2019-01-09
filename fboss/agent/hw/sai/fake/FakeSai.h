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

#include "FakeSaiBridge.h"
#include "FakeSaiLag.h"
#include "FakeSaiNextHop.h"
#include "FakeSaiNeighbor.h"
#include "FakeSaiPort.h"
#include "FakeSaiRoute.h"
#include "FakeSaiRouterInterface.h"
#include "FakeSaiSwitch.h"
#include "FakeSaiVirtualRouter.h"
#include "FakeSaiVlan.h"

#include <memory>

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

struct FakeSai {
  static std::shared_ptr<FakeSai> getInstance();
  FakeBridgeManager brm;
  FakeLagManager lm;
  FakeNeighborManager nm;
  FakeNextHopManager nhm;
  FakePortManager pm;
  FakeRouteManager rm;
  FakeRouterInterfaceManager rim;
  FakeSwitchManager swm;
  FakeVirtualRouterManager vrm;
  FakeVlanManager vm;
  bool initialized = false;
};

} // namespace fboss
} // namespace facebook

sai_status_t sai_api_initialize(
    uint64_t flags,
    const sai_service_method_table_t* services);

sai_status_t sai_api_query(sai_api_t sai_api_id, void** api_method_table);
