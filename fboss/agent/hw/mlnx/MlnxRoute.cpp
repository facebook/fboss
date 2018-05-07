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
#include "fboss/agent/hw/mlnx/MlnxRoute.h"
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"
#include "fboss/agent/hw/mlnx/MlnxIntf.h"
#include "fboss/agent/hw/mlnx/MlnxIntfTable.h"
#include "fboss/agent/hw/mlnx/MlnxVrf.h"
#include "fboss/agent/hw/mlnx/MlnxUtils.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"

#include "fboss/agent/FbossError.h"

extern "C" {
#include <sx/sdk/sx_api_router.h>
}

#include <folly/Format.h>
#include <folly/IPAddress.h>

namespace facebook { namespace fboss {

bool MlnxRouteKey::operator <(const MlnxRouteKey& key) const {
  if (vrf < key.vrf) {
    return true;
  } else if (vrf > key.vrf) {
    return false;
  }
  return (network < key.network);
}

MlnxRoute::MlnxRoute(MlnxSwitch* hw,
  const MlnxRouteKey& key,
  const RouteNextHopEntry& fwd) :
  hw_(hw),
  handle_(hw_->getHandle()),
  vrid_(hw_->getDefaultMlnxVrf()->getSdkVrid()),
  network_(key.network),
  fwd_(fwd) {
  // Convert folly network to SDK prefix structure
  utils::follyIPPrefixToSdk(network_, &prefix_);

  // program to hardware based on forwarding information (action, next hops...)
  program(SX_ACCESS_CMD_ADD, fwd);

  auto logString = toString();

  LOG(INFO) << "Programmed route to HW: " << logString;
}

void MlnxRoute::program(sx_access_cmd_t cmd,
  const RouteNextHopEntry& fwd) {
  sx_status_t rc;
  // fill route data
  makeRouteData(fwd, &routeData_);

  // program to HW
  rc = sx_api_router_uc_route_set(handle_,
    cmd,
    vrid_,
    &prefix_,
    &routeData_);
  mlnxCheckSdkFail(rc,
    "sx_api_router_uc_route_set",
    "Failed to ",
    SX_ACCESS_CMD_STR(cmd),
    " route:",
    toString());
}

void MlnxRoute::change(const RouteNextHopEntry& fwd) {
  auto logStringOldRoute = toString();

  // fill route data
  program(SX_ACCESS_CMD_SET, fwd);

  LOG(INFO) << "Changed route in HW: "
            << logStringOldRoute
            << " => "
            << toString();

  // save forwarding info
  fwd_ = fwd;
}

void MlnxRoute::makeRouteData(const RouteNextHopEntry& fwd,
  sx_uc_route_data_t* routeData) const {
  const auto& nhopSet = fwd.getNextHopSet();
  MlnxIntf* intf {nullptr};

  // fill routeData based on action
  switch(fwd.getAction()) {
  case RouteNextHopEntry::Action::DROP:
    makeDropRouteData(routeData);
    break;
  case RouteNextHopEntry::Action::TO_CPU:
    makeToCPURouteData(routeData);
    break;
  case RouteNextHopEntry::Action::NEXTHOPS:
    // in case of local route nhopSet contains interface IP,
    // which should be in subnet of route network ip.
    // find the interface with ip equals to nhop ip
    if (nhopSet.size() == 1 &&
        nhopSet.begin()->addr().inSubnet(network_.first, network_.second)) {
      intf = hw_->getMlnxIntfTable()->getMlnxIntfByAttachedIpIf(
        nhopSet.begin()->addr());
    }

    // if such interface was found it is local route
    if (intf) {
      auto rif = intf->getSdkRif();
      makeLocalRouteData(rif, routeData);
    } else {
      // next hop route
      makeNexthopRouteData(nhopSet, routeData);
    }
    break;
  default:
    throw FbossError("Not supported action for route: ",
      toString());
    break;
  }
}

void MlnxRoute::makeToCPURouteData(sx_uc_route_data_t* routeData) const {
  routeData->action = SX_ROUTER_ACTION_TRAP;
  // trap priority always medium for POC
  routeData->trap_attr.prio = SX_TRAP_PRIORITY_MED;
  routeData->type = SX_UC_ROUTE_TYPE_NEXT_HOP;
  routeData->uc_route_param.ecmp_id = SX_ROUTER_ECMP_ID_INVALID;
}

void MlnxRoute::makeDropRouteData(sx_uc_route_data_t* routeData) const {
  routeData->action = SX_ROUTER_ACTION_DROP;
  routeData->type = SX_UC_ROUTE_TYPE_NEXT_HOP;
  routeData->uc_route_param.ecmp_id = SX_ROUTER_ECMP_ID_INVALID;
}

void MlnxRoute::makeNexthopRouteData(
  const RouteNextHopEntry::NextHopSet& nextHopsSet,
  sx_uc_route_data_t* routeData) const {
  if (nextHopsSet.size() > RM_API_ROUTER_NEXT_HOP_MAX) {
    throw FbossError("Max number of next hops for route is ",
      static_cast<int>(RM_API_ROUTER_NEXT_HOP_MAX),
      "; ",
      toString());
  }
  // action set to forward
  routeData->action = SX_ROUTER_ACTION_FORWARD;
  // next hop
  routeData->type = SX_UC_ROUTE_TYPE_NEXT_HOP;
  int nhopCounter = 0;
  // Probably in future we will use external ECMP container for
  // next hop routes and create a Mlnx* instance that would manage ECMP
  // container, for POC use simplier approach
  for (const auto& nextHop: nextHopsSet) {
    // convert address to SDK structure and write to next_hop_list_p
    utils::follyIPAddressToSdk(nextHop.addr(),
      routeData->next_hop_list_p + nhopCounter);
    ++nhopCounter;
  }
  routeData->next_hop_cnt = nhopCounter;
  // Invalid ecmp id in this case
  routeData->uc_route_param.ecmp_id = SX_ROUTER_ECMP_ID_INVALID;
}

void MlnxRoute::makeLocalRouteData(sx_router_interface_t rif,
  sx_uc_route_data_t* routeData) const {
  // action set to forward
  routeData->action = SX_ROUTER_ACTION_FORWARD;
  routeData->type = SX_UC_ROUTE_TYPE_LOCAL;
  routeData->uc_route_param.local_egress_rif = rif;
}

std::string MlnxRoute::toString() const {
  return folly::sformat("vrid {:d}, rotue {} via {}",
      vrid_, folly::IPAddress::networkToString(network_), fwd_.str());
}

MlnxRoute::~MlnxRoute() {
  sx_status_t rc;

  auto logString = toString();
  // delete route from HW
  rc = sx_api_router_uc_route_set(handle_,
    SX_ACCESS_CMD_DELETE,
    vrid_,
    &prefix_,
    nullptr);
  mlnxLogSxError(rc,
    "sx_api_router_uc_route_set",
    "Failed to delete route network: ",
    logString);

  LOG_IF(INFO, rc == SX_STATUS_SUCCESS) << "Deleted route from HW: "
                                        << logString;
}

}} // facebook::fboss
