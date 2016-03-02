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
#include "SaiHost.h"
#include "SaiVrfTable.h"
#include "SaiVrf.h"
#include "SaiIntfTable.h"
#include "SaiIntf.h"
#include "SaiSwitch.h"
#include "SaiError.h"

using folly::IPAddress;
using folly::MacAddress;

namespace facebook { namespace fboss {

SaiHost::SaiHost(const SaiSwitch *hw, InterfaceID intf,
                 const IPAddress &ip, const folly::MacAddress &mac)
  : hw_(hw)
  , intf_(intf)
  , ip_(ip)
  , mac_(mac) {

  VLOG(4) << "Entering " << __FUNCTION__;

  pSaiNeighborApi_ = hw_->GetSaiNeighborApi();
}

SaiHost::~SaiHost() {
  VLOG(4) << "Entering " << __FUNCTION__;

  if (!added_) {
    return;
  }

  pSaiNeighborApi_->remove_neighbor_entry(&neighborEntry_);
  VLOG(3) << "deleted L3 host object for " << ip_;
}

void SaiHost::Program(sai_packet_action_t action) {
  VLOG(4) << "Entering " << __FUNCTION__;

  if (!added_) {

    if (ip_.empty() || ip_.isZero()) {
      throw SaiError("Wrong IP address ", ip_, " specified");
    }

    // fill neighborEntry_
    neighborEntry_.rif_id = hw_->GetIntfTable()->GetIntf(intf_)->GetIfId();

    if (ip_.isV4()) {
      // IPv4
      neighborEntry_.ip_address.addr_family = SAI_IP_ADDR_FAMILY_IPV4;

      memcpy(&neighborEntry_.ip_address.addr.ip4, ip_.bytes(),
             sizeof(neighborEntry_.ip_address.addr.ip4));
    } else {
      // IPv6
      neighborEntry_.ip_address.addr_family = SAI_IP_ADDR_FAMILY_IPV6;

      memcpy(neighborEntry_.ip_address.addr.ip6, ip_.bytes(),
             sizeof(neighborEntry_.ip_address.addr.ip6));
    }

    // fill neighbor attributes
    sai_attribute_t attr_list[SAI_NEIGHBOR_ATTR_COUNT] = {0};

    // MAC
    attr_list[0].id = SAI_NEIGHBOR_ATTR_DST_MAC_ADDRESS;
    memcpy(attr_list[0].value.mac, mac_.bytes(), sizeof(attr_list[0].value.mac));

    // packet action
    attr_list[1].id = SAI_NEIGHBOR_ATTR_PACKET_ACTION;
    attr_list[1].value.u32 = action;

    // create neighbor
    sai_status_t saiRetVal = pSaiNeighborApi_->create_neighbor_entry(&neighborEntry_,
                                                                     SAI_NEIGHBOR_ATTR_COUNT,
                                                                     attr_list);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
       throw SaiError("Could not create SAI neighbor with IP addr: ", ip_,
                        " MAC: ", mac_, " router interface ID: ", intf_,
                        " Error: ", saiRetVal);
    }

    added_ = true;
    action_ = action;

    return;

  } else if (action == action_) {
    // nothing to do more
    return;
  }

  // fill neighbor packet action attribute
  sai_attribute_t action_attr = {0};
  action_attr.id = SAI_NEIGHBOR_ATTR_PACKET_ACTION;
  action_attr.value.u32 = action;

  // set action attribute
  sai_status_t saiRetVal = pSaiNeighborApi_->set_neighbor_attribute(&neighborEntry_,
                                                                    &action_attr);
  if (saiRetVal != SAI_STATUS_SUCCESS) {
    throw SaiError("Could not set packet action attribute: ", action,
                     "to SAI neighbor with IP addr: ", ip_,
                     " MAC: ", mac_, " router interface ID: ", intf_,
                     " Error: ", saiRetVal);
  }

  action_ = action;
}

}} // facebook::fboss
