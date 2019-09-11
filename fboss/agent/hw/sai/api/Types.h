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

FBOSS_STRONG_TYPE(sai_object_id_t, BridgeSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, BridgePortSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, HostifTrapGroupSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, HostifTrapSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, NextHopSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, NextHopGroupSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, NextHopGroupMemberSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, PortSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, QueueSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, RouterInterfaceSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, SchedulerSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, SwitchSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, VirtualRouterSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, VlanSaiId);
FBOSS_STRONG_TYPE(sai_object_id_t, VlanMemberSaiId);

namespace facebook {
namespace fboss {

template <typename SaiId>
sai_object_id_t* rawSaiId(SaiId* id) {
  return reinterpret_cast<sai_object_id_t*>(id);
}

} // namespace fboss
} // namespace facebook
