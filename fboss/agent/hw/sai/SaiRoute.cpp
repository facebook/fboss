/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include "SaiRoute.h"
#include "SaiVrfTable.h"
#include "SaiVrf.h"
#include "SaiIntfTable.h"
#include "SaiIntf.h"
#include "SaiNextHopTable.h"
#include "SaiNextHop.h"
#include "SaiSwitch.h"
#include "SaiError.h"

namespace facebook { namespace fboss {

SaiRoute::SaiRoute(const SaiSwitch *hw,
                   RouterID vrf,
                   const folly::IPAddress &addr,
                   uint8_t prefixLen)
  : hw_(hw)
  , vrf_(vrf)
  , ipAddr_(addr)
  , prefixLen_(prefixLen) {

  VLOG(4) << "Entering " << __FUNCTION__;

  auto intfPtr = hw_->GetIntfTable()->GetFirstIntfIf();
  while (intfPtr != nullptr) {
    for (const auto& addrPair : intfPtr->GetInterface()->getAddresses()) {
      // If route IP adress is in the same subnet as an IP of one of the L3 interfaces
      // then we deal with local route
      if (addrPair.first.inSubnet(addr, prefixLen)) {
          isLocal_ = true;
          break;
      }
    }
    intfPtr = hw_->GetIntfTable()->GetNextIntfIf(intfPtr);
  }

  pSaiRouteApi_ = hw->GetSaiRouteApi();
}

SaiRoute::~SaiRoute() {
  VLOG(4) << "Entering " << __FUNCTION__;

  if(!added_) {
    return;
  }

  sai_status_t saiRetVal = pSaiRouteApi_->remove_route(&routeEntry_);
  if (saiRetVal != SAI_STATUS_SUCCESS) {
    LOG(ERROR) << "SAI remove_route() failed for route with address: "
               << ipAddr_.str() << "/" << prefixLen_ << " Error: " << saiRetVal;
  }

  if (fwd_.getAction() == RouteForwardAction::NEXTHOPS) {
    // derefernce next hop
    hw_->WritableNextHopTable()->DerefSaiNextHop(fwd_);
  }

  // dereference vrf_
  hw_->WritableVrfTable()->DerefSaiVrf(vrf_);
}

void SaiRoute::Program(const RouteForwardInfo &fwd) {
  VLOG(4) << "Entering " << __FUNCTION__;

  if (isLocal_) {
    // FBOSS creates routes for local IP adresses of L3 interfaces
    // For example if IP address of an interface is 10.10.10.1/24
    // there will be the next route created: 10.10.10.0 => 10.10.10.1/24.
    // If such a route is programmed in HW all the traffic destined to other hosts 
    // from the same subnet will be dropped or trapped to cpu. 
    // In order to avoid this just return without programming to hardware.
    VLOG(2) << "Local route " << ipAddr_ << "/" << prefixLen_ << " => " 
            << fwd << " will not be programmed to HW";
    return;
  }

  auto newAction = fwd.getAction();

  if ((newAction == RouteForwardAction::NEXTHOPS) && fwd.getNexthops().empty()) {
      throw SaiError("RouteForwardInfo has empty next hops list");
  }

  if (added_) {
    // if the route has been programmed to the HW and the forward info is
    // not changed nothing to do.
    if (fwd == fwd_) {
      return;
    }

    if (fwd_.getAction() == RouteForwardAction::NEXTHOPS) {
      // derefernce old next hop
      hw_->WritableNextHopTable()->DerefSaiNextHop(fwd_);
    }
  } else {
    // vrf as well as the route itself has to be referenced/created only
    // for the first time
    auto vrf = hw_->WritableVrfTable()->IncRefOrCreateSaiVrf(vrf_);

    try {
      vrf->Program();
    } catch (const SaiError &e) {
      hw_->WritableVrfTable()->DerefSaiVrf(vrf_);
      // let handle this in the caller.
      throw;
    }

    routeEntry_.vr_id = vrf->GetSaiVrfId();

    if (ipAddr_.empty()) {
      throw SaiError("Could not program route with empty subnet IP address");
    }

    if ((!ipAddr_.isZero() && (prefixLen_ == 0)) || prefixLen_ > ipAddr_.bitCount()) {
      throw SaiError("Wrong subnet IP prefix length ", prefixLen_, " specified");
    }

    // fill routeEntry_
    if (ipAddr_.isV4()) {
      // IPv4
      routeEntry_.destination.addr_family = SAI_IP_ADDR_FAMILY_IPV4;

      memcpy(&routeEntry_.destination.addr.ip4, ipAddr_.bytes(),
             sizeof(routeEntry_.destination.addr.ip4));

      const uint8_t* mask = nullptr;
      if (ipAddr_.isZero() && (prefixLen_ == 0)) {
        // default gateway case
        static const uint8_t zeroMask[4] = {0};
        mask = zeroMask;
      } else {
        mask = folly::IPAddressV4::fetchMask(prefixLen_).data();
      }

      memcpy(&routeEntry_.destination.mask.ip4, mask,
             sizeof(routeEntry_.destination.mask.ip4));
    } else {
      // IPv6
      routeEntry_.destination.addr_family = SAI_IP_ADDR_FAMILY_IPV6;

      memcpy(routeEntry_.destination.addr.ip6, ipAddr_.bytes(),
             sizeof(routeEntry_.destination.addr.ip6));

      const uint8_t* mask = nullptr;
      if (ipAddr_.isZero() && (prefixLen_ == 0)) {
        // default gateway case
        static const uint8_t zeroMask[128] = {0};
        mask = zeroMask;
      } else {
          mask = folly::IPAddressV6::fetchMask(prefixLen_).data();
      }

      memcpy(routeEntry_.destination.mask.ip6, mask,
             sizeof(routeEntry_.destination.mask.ip6));
    }

    // create route
    sai_status_t saiRetVal = pSaiRouteApi_->create_route(&routeEntry_, 0, NULL);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
       throw SaiError("Could not create route with addr: ", ipAddr_,
                        " netmask length: ", prefixLen_, " vrf: ", vrf_,
                        " Error: ", saiRetVal);
    }

    added_ = true;
  }

  sai_attribute_t routeAttr = {0};

  // fill atributes
  // route packet action
  routeAttr.id = SAI_ROUTE_ATTR_PACKET_ACTION;

  switch (newAction) {

  case RouteForwardAction::DROP:
    routeAttr.value.u32 = SAI_PACKET_ACTION_DROP;
    break;

  case RouteForwardAction::TO_CPU:
    routeAttr.value.u32 = SAI_PACKET_ACTION_TRAP;
    break;

  case RouteForwardAction::NEXTHOPS:
    routeAttr.value.u32 = SAI_PACKET_ACTION_FORWARD;
    break;

  default:
    throw SaiError("Unknown RouteForwardAction ", newAction, " specified");
  }

  // Set packet action route attribute in SAI
  sai_status_t saiRetVal = pSaiRouteApi_->set_route_attribute(&routeEntry_, &routeAttr);
  if (saiRetVal != SAI_STATUS_SUCCESS) {
    throw SaiError("Could not set packet action attribute for route with addr: ", ipAddr_,
                     " netmask length: ", prefixLen_, " vrf: ", vrf_,
                     " Error: ", saiRetVal);
  }

  if (newAction == RouteForwardAction::NEXTHOPS) {
    // fill Next Hop ID
    routeAttr.id = SAI_ROUTE_ATTR_NEXT_HOP_ID;
    routeAttr.value.oid = hw_->WritableNextHopTable()->IncRefOrCreateSaiNextHop(fwd)->GetSaiNextHopId();

    // Set next hop ID route attribute in SAI
    saiRetVal = pSaiRouteApi_->set_route_attribute(&routeEntry_, &routeAttr);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
      throw SaiError("Could not set next hop ID attribute for route with addr: ", ipAddr_,
                       " netmask length: ", prefixLen_, " vrf: ", vrf_,
                       " Error: ", saiRetVal);
    }
  }

  fwd_ = fwd;
}

}} // facebook::fboss
