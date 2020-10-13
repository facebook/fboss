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

#include "fboss/agent/types.h"

extern "C" {
#include <sai.h>
}

FBOSS_STRONG_TYPE(sai_object_id_t, AclTableGroupSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, AclTableGroupMemberSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, AclTableSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, AclEntrySaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, AclCounterSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, BridgeSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, BridgePortSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, BufferPoolSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, BufferProfileSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, DebugCounterSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, HashSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, HostifTrapGroupSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, HostifTrapSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, NextHopSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, NextHopGroupSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, NextHopGroupMemberSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, PortSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, PortSerdesSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, QosMapSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, QueueSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, RouterInterfaceSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, SchedulerSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, SwitchSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, VirtualRouterSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, VlanSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, VlanMemberSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, WredSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, TamReportSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, TamEventActionSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, TamEventSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, TamSaiId);

namespace facebook::fboss {

template <typename SaiId>
sai_object_id_t* rawSaiId(SaiId* id) {
  return reinterpret_cast<sai_object_id_t*>(id);
}

} // namespace facebook::fboss
