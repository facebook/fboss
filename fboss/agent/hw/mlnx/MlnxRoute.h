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

#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

#include <folly/Optional.h>
#include <folly/IPAddress.h>

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_router.h>
}

namespace facebook { namespace fboss {

class MlnxSwitch;

/**
 * This structure is used as a key for route table
 * Contains Router ID and network prefix
 */
struct MlnxRouteKey {
  MlnxRouteKey(RouterID vrf, const folly::CIDRNetwork& network):
    vrf(vrf),
    network(network){}

  RouterID vrf;
  folly::CIDRNetwork network;

  /**
   * Comparison operator, required for boost::container::flat_map
   * implementation, to keep the map sorted
   *
   * @param key Another MlnxRouteKey instance
   * @ret true if less then @key, otherwise false
   */
  bool operator < (const MlnxRouteKey& key) const;
};

/**
 * This class abstracts route in SDK
 */
class MlnxRoute {
 public:

  /**
   * ctor, creates route in SDK;
   * Route entry lifetime in SDK is bound to the lifetime
   * of this object
   *
   * @param hw Pointer to MlnxSwitch
   * @param key Route key
   * @param fwd Forwarding information
   */
  MlnxRoute(MlnxSwitch* hw,
    const MlnxRouteKey& key,
    const RouteNextHopEntry& fwd);

  /**
   * Modify route based on route information
   *
   * @param fwd Forwarding information
   */
  void change(const RouteNextHopEntry& fwd);

  /**
   * Serialize SDK route data into string
   * Useful for logging and debugging
   *
   * @ret string representation of a route
   */
  std::string toString() const;

  /**
   * dtor, deletes route form HW
   */
  ~MlnxRoute();
 private:
  // forbidden copy constructor and assignment operator
  MlnxRoute(const MlnxRoute&) = delete;
  MlnxRoute& operator=(const MlnxRoute&) = delete;

  /**
   * Program route to HW
   *
   * @param cmd command to apply to route (ADD or SET)
   * @param fwd Forwarding information
   */
  void program(sx_access_cmd_t cmd,
    const RouteNextHopEntry& fwd);

  /**
   * Fill sx_uc_route_data_t structure base on
   * route forwarding information
   *
   * @param[in] fwd Forwarding information
   * @param[out] routeData Pointer to sx_uc_route_data_t structure
   */
  void makeRouteData(const RouteNextHopEntry& fwd,
    sx_uc_route_data_t* routeData) const;

  /**
   * Fill sx_uc_route_data_t structure for UC route with action ToCPU
   *
   * @param[out] routeData Pointer to sx_uc_route_data_t structure
   */
  void makeToCPURouteData(sx_uc_route_data_t* routeData) const;

  /**
   * Fill sx_uc_route_data_t structure for UC route with action DROP
   *
   * @param[out] routeData Pointer to sx_uc_route_data_t structure
   */
  void makeDropRouteData(sx_uc_route_data_t* routeData) const;

  /**
   * Fill sx_uc_route_data_t structure for UC route type NEXTHOP
   *
   * @param[in] nextHopsSet Set of next hops or IP address of an interface
   * @param[out] routeData Pointer to sx_uc_route_data_t structure
   */
  void makeNexthopRouteData(const RouteNextHopEntry::NextHopSet& nextHopsSet,
    sx_uc_route_data_t* routeData) const;

  /**
   * Fill sx_uc_route_data_t structure for UC route type LOCAL
   *
   * @param[in] rif router interface id
   * @param[out] routeData Pointer to sx_uc_route_data_t structure
   */
  void makeLocalRouteData(sx_router_interface_t rif,
    sx_uc_route_data_t* routeData) const;

  // private fields

  MlnxSwitch* hw_ {nullptr};
  sx_api_handle_t handle_ {SX_API_INVALID_HANDLE};
  sx_router_id_t vrid_;
  sx_ip_prefix prefix_;
  sx_uc_route_data routeData_;
  folly::CIDRNetwork network_;
  RouteNextHopEntry fwd_;
};

}} // facebook::fboss
