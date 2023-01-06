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

#include <folly/concurrency/ConcurrentHashMap.h>
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/types.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

struct ConcurrentIndices {
  ~ConcurrentIndices();
  /*
   * portIds and vlanIds are read by rx packet processing
   * and modified by port/vlan updates
   */
  folly::ConcurrentHashMap<PortSaiId, PortID> portIds;
  // indexed by port sai id(or lag sai id), not vlan sai id, until sai
  // callback supports punt with vlan id in either an attribute
  // or the frame itself
  folly::ConcurrentHashMap<PortDescriptorSaiId, VlanID> vlanIds;
  /*
   * Indexed by PortID, used by TX to translate port ID
   * to sai port id.
   */
  folly::ConcurrentHashMap<PortID, PortSaiId> portSaiIds;
  /*
   * port sai id of a port which is member of aggregate port
   */
  folly::ConcurrentHashMap<PortSaiId, AggregatePortID>
      memberPort2AggregatePortIds;
  /*
   * lag sai id to aggregate port id
   */
  folly::ConcurrentHashMap<LagSaiId, AggregatePortID> aggregatePortIds;

  /*
   * Config Hostif trap ID to Rx reason
   */
  folly::ConcurrentHashMap<HostifTrapSaiId, cfg::PacketRxReason> hostifTrapIds;

  /*
   * sys port sai id to SystemPortId
   */
  folly::ConcurrentHashMap<SystemPortSaiId, SystemPortID> sysPortIds;
  /*
   * SystemPortID to sai sys port id
   */
  folly::ConcurrentHashMap<SystemPortID, SystemPortSaiId> sysPortSaiIds;
};

} // namespace facebook::fboss
