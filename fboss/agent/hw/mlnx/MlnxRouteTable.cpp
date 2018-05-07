/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "fboss/agent/hw/mlnx/MlnxRouteTable.h"
#include "fboss/agent/hw/mlnx/MlnxRoute.h"
#include "fboss/agent/hw/mlnx/MlnxIntfTable.h"
#include "fboss/agent/hw/mlnx/MlnxIntf.h"

namespace facebook { namespace fboss {

MlnxRouteTable::MlnxRouteTable (MlnxSwitch* hw) :
  hw_(hw) {
}

MlnxRouteTable::~MlnxRouteTable () {
}

template<typename RouteT>
void MlnxRouteTable::addRoute(RouterID vrf,
  const std::shared_ptr<RouteT>& swRoute) {
  const auto& prefix = swRoute->prefix();
  folly::CIDRNetwork network {prefix.network, prefix.mask};
  MlnxRouteKey key(vrf, network);
  auto route = std::make_unique<MlnxRoute>(hw_,
    key,
    swRoute->getForwardInfo());
  routes_.emplace(key, std::move(route));
}

template<typename RouteT>
void MlnxRouteTable::changeRoute(RouterID vrf,
  const std::shared_ptr<RouteT>& oldRoute,
  const std::shared_ptr<RouteT>& newRoute) {
  const auto& prefix = newRoute->prefix();
  folly::CIDRNetwork network {prefix.network, prefix.mask};
  MlnxRouteKey key(vrf, network);
  MlnxRoute* route = routes_.find(key)->second.get();
  route->change(newRoute->getForwardInfo());
}

template<typename RouteT>
void MlnxRouteTable::deleteRoute(RouterID vrf,
  const std::shared_ptr<RouteT>& swRoute) {
  const auto& prefix = swRoute->prefix();
  folly::CIDRNetwork network {prefix.network, prefix.mask};
  MlnxRouteKey key(vrf, network);
  routes_.erase(key);
}

// force compiler to generate code for RouteT=RouteV4 and RouteT=RouteV6

template void MlnxRouteTable::addRoute(RouterID,
  const std::shared_ptr<RouteV4>&);
template void MlnxRouteTable::addRoute(RouterID,
  const std::shared_ptr<RouteV6>&);

template void MlnxRouteTable::deleteRoute(RouterID,
  const std::shared_ptr<RouteV4>&);
template void MlnxRouteTable::deleteRoute(RouterID,
  const std::shared_ptr<RouteV6>&);

template void MlnxRouteTable::changeRoute(RouterID,
  const std::shared_ptr<RouteV4>&,
  const std::shared_ptr<RouteV4>&);
template void MlnxRouteTable::changeRoute(RouterID,
  const std::shared_ptr<RouteV6>&,
  const std::shared_ptr<RouteV6>&);

}} // facebook::fboss
