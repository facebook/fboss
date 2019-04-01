/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SaiApiTable.h"

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {
SaiApiTable::SaiApiTable() {
  sai_api_initialize(0, nullptr);
  bridgeApi_ = std::make_unique<BridgeApi>();
  fdbApi_ = std::make_unique<FdbApi>();
  nextHopApi_ = std::make_unique<NextHopApi>();
  nextHopGroupApi_ = std::make_unique<NextHopGroupApi>();
  neighborApi_ = std::make_unique<NeighborApi>();
  portApi_ = std::make_unique<PortApi>();
  routeApi_ = std::make_unique<RouteApi>();
  routerInterfaceApi_ = std::make_unique<RouterInterfaceApi>();
  switchApi_ = std::make_unique<SwitchApi>();
  virtualRouterApi_ = std::make_unique<VirtualRouterApi>();
  vlanApi_ = std::make_unique<VlanApi>();
}
}
}
