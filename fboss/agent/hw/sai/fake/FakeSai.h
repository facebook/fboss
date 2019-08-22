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

#include "fboss/agent/hw/sai/fake/FakeSaiBridge.h"
#include "fboss/agent/hw/sai/fake/FakeSaiFdb.h"
#include "fboss/agent/hw/sai/fake/FakeSaiHostif.h"
#include "fboss/agent/hw/sai/fake/FakeSaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/fake/FakeSaiLag.h"
#include "fboss/agent/hw/sai/fake/FakeSaiNeighbor.h"
#include "fboss/agent/hw/sai/fake/FakeSaiNextHop.h"
#include "fboss/agent/hw/sai/fake/FakeSaiNextHopGroup.h"
#include "fboss/agent/hw/sai/fake/FakeSaiPort.h"
#include "fboss/agent/hw/sai/fake/FakeSaiQueue.h"
#include "fboss/agent/hw/sai/fake/FakeSaiRoute.h"
#include "fboss/agent/hw/sai/fake/FakeSaiRouterInterface.h"
#include "fboss/agent/hw/sai/fake/FakeSaiScheduler.h"
#include "fboss/agent/hw/sai/fake/FakeSaiSwitch.h"
#include "fboss/agent/hw/sai/fake/FakeSaiVirtualRouter.h"
#include "fboss/agent/hw/sai/fake/FakeSaiVlan.h"

#include <memory>

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

struct FakeSai {
  static std::shared_ptr<FakeSai> getInstance();
  FakeBridgeManager brm;
  FakeFdbManager fdbm;
  FakeHostifTrapManager htm;
  FakeHostifTrapGroupManager htgm;
  FakeLagManager lm;
  FakeInSegEntryManager inSegEntryManager;
  FakeNeighborManager nm;
  FakeNextHopManager nhm;
  FakeNextHopGroupManager nhgm;
  FakePortManager pm;
  FakeQueueManager qm;
  FakeRouteManager rm;
  FakeRouterInterfaceManager rim;
  FakeSchedulerManager scm;
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
