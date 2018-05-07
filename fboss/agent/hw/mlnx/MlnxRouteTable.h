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
#pragma once

#include "fboss/agent/hw/mlnx/MlnxRoute.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>

#include <folly/IPAddress.h>

#include <memory>

namespace facebook { namespace fboss {

class MlnxSwitch;

class MlnxRouteTable {
 public:
  /**
   * RouteMapT is a map between MlnxRouteKey (router ID, network/mask) and
   * MlnxRoute instance
   */
  using RouteMapT =
    boost::container::flat_map<MlnxRouteKey,std::unique_ptr<MlnxRoute>>;

  /**
   * ctor, empty map
   *
   * @param hw Pointer to MlnxSwitch
   */

  MlnxRouteTable(MlnxSwitch* hw);

  /**
   * dtor, deletes the map
   */
  ~MlnxRouteTable();

  /**
   * Adds a new route to a map
   * RouteT - a route type (RouteV4, RouteV6)
   *
   * @param vrf Router ID
   * @param swRoute SW Route instance
   */
  template<typename RouteT>
  void addRoute(RouterID vrf, const std::shared_ptr<RouteT>& swRoute);

  /**
   * Changes exsiting route in table, throw if not found
   * RouteT - a route type (RouteV4, RouteV6)
   *
   * @param vrf Router ID
   * @param oldRoute SW old route instance
   * @param newRoute SW new route instance
   */
  template<typename RouteT>
  void changeRoute(RouterID vrf,
    const std::shared_ptr<RouteT>& oldRoute,
    const std::shared_ptr<RouteT>& newRoute);

  /**
   * Deletes a new route from a map
   * RouteT - a route type (RouteV4, RouteV6)
   *
   * @param vrf Router ID
   * @param swRoute SW Route instance to delete
   */
  template<typename RouteT>
  void deleteRoute(RouterID vrf, const std::shared_ptr<RouteT>& swRoute);

 private:
   // forbidden copy constructor and assignment operator
   MlnxRouteTable(const MlnxRouteTable&) = delete;
   MlnxRouteTable& operator=(const MlnxRouteTable&) = delete;

  // private fields
  MlnxSwitch* hw_ {nullptr};
  RouteMapT routes_ {};
};

}} // facebook::fboss
