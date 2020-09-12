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

#include "fboss/agent/hw/sai/fake/FakeSaiAcl.h"
#include "fboss/agent/hw/sai/fake/FakeSaiBridge.h"
#include "fboss/agent/hw/sai/fake/FakeSaiBuffer.h"
#include "fboss/agent/hw/sai/fake/FakeSaiDebugCounter.h"
#include "fboss/agent/hw/sai/fake/FakeSaiFdb.h"
#include "fboss/agent/hw/sai/fake/FakeSaiHash.h"
#include "fboss/agent/hw/sai/fake/FakeSaiHostif.h"
#include "fboss/agent/hw/sai/fake/FakeSaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/fake/FakeSaiNeighbor.h"
#include "fboss/agent/hw/sai/fake/FakeSaiNextHop.h"
#include "fboss/agent/hw/sai/fake/FakeSaiNextHopGroup.h"
#include "fboss/agent/hw/sai/fake/FakeSaiPort.h"
#include "fboss/agent/hw/sai/fake/FakeSaiQosMap.h"
#include "fboss/agent/hw/sai/fake/FakeSaiQueue.h"
#include "fboss/agent/hw/sai/fake/FakeSaiRoute.h"
#include "fboss/agent/hw/sai/fake/FakeSaiRouterInterface.h"
#include "fboss/agent/hw/sai/fake/FakeSaiScheduler.h"
#include "fboss/agent/hw/sai/fake/FakeSaiSwitch.h"
#include "fboss/agent/hw/sai/fake/FakeSaiVirtualRouter.h"
#include "fboss/agent/hw/sai/fake/FakeSaiVlan.h"
#include "fboss/agent/hw/sai/fake/FakeSaiWred.h"

#include <memory>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

struct FakeSai {
  static std::shared_ptr<FakeSai> getInstance();
  static void clear();

  FakeAclTableGroupManager aclTableGroupManager;
  FakeAclEntryManager aclEntryManager;
  FakeAclCounterManager aclCounterManager;
  FakeAclTableManager aclTableManager;
  FakeBridgeManager bridgeManager;
  FakeBufferPoolManager bufferPoolManager;
  FakeBufferProfileManager bufferProfileManager;
  FakeDebugCounterManager debugCounterManager;
  FakeFdbManager fdbManager;
  FakeHashManager hashManager;
  FakeHostifTrapManager hostIfTrapManager;
  FakeHostifTrapGroupManager hostifTrapGroupManager;
  FakeInSegEntryManager inSegEntryManager;
  FakeNeighborManager neighborManager;
  FakeNextHopManager nextHopManager;
  FakeNextHopGroupManager nextHopGroupManager;
  FakePortManager portManager;
  FakeQosMapManager qosMapManager;
  FakeQueueManager queueManager;
  FakeRouteManager routeManager;
  FakeRouterInterfaceManager routeInterfaceManager;
  FakeSchedulerManager scheduleManager;
  FakeSwitchManager switchManager;
  FakeVirtualRouterManager virtualRouteManager;
  FakeVlanManager vlanManager;
  FakeWredManager wredManager;
  bool initialized = false;
  sai_object_id_t cpuPortId;
  sai_object_id_t getCpuPort();
};

} // namespace facebook::fboss

sai_status_t sai_api_initialize(
    uint64_t flags,
    const sai_service_method_table_t* services);

sai_status_t sai_api_query(sai_api_t sai_api_id, void** api_method_table);

sai_status_t sai_log_set(sai_api_t api, sai_log_level_t log_level);

sai_status_t sai_dbg_generate_dump(const char* dump_file_name);
