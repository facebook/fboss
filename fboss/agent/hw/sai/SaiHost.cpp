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

  saiNeighborApi_ = hw_->getSaiNeighborApi();
}

SaiHost::~SaiHost() {
  VLOG(4) << "Entering " << __FUNCTION__;

  if (!added_) {
    return;
  }

  saiNeighborApi_->remove_neighbor_entry(&neighborEntry_);
  VLOG(3) << "Deleted L3 host object for " << ip_;
}

void SaiHost::program(sai_packet_action_t action, const folly::MacAddress &mac) {
  VLOG(4) << "Entering " << __FUNCTION__;

  if (!added_) {

    if (ip_.empty() || ip_.isZero()) {
      throw SaiError("Wrong IP address ", ip_, " specified");
    }

    // fill neighborEntry_
    neighborEntry_.rif_id = hw_->getIntfTable()->getIntf(intf_)->getIfId();

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
    std::vector<sai_attribute_t> attrList;
    sai_attribute_t attr {0};

    // MAC
    attr.id = SAI_NEIGHBOR_ATTR_DST_MAC_ADDRESS;
    memcpy(attr.value.mac, mac_.bytes(), sizeof(attr.value.mac));
    attrList.push_back(attr);

    // packet action
    attr.id = SAI_NEIGHBOR_ATTR_PACKET_ACTION;
    attr.value.u32 = action;
    attrList.push_back(attr);

    // create neighbor
    sai_status_t saiRetVal = saiNeighborApi_->create_neighbor_entry(&neighborEntry_,
                                                                     attrList.size(),
                                                                     attrList.data());
    if (saiRetVal != SAI_STATUS_SUCCESS) {
       throw SaiError("Could not create SAI neighbor with IP addr: ", ip_,
                       " MAC: ", mac_, " router interface ID: ", intf_,
                       " Error: ", saiRetVal);
    }

    added_ = true;
    action_ = action;

    return;
  }

  if (mac != mac_) {
    // fill neighbor MAC attribute
    sai_attribute_t macAttr {0};
    macAttr.id = SAI_NEIGHBOR_ATTR_DST_MAC_ADDRESS;
    memcpy(macAttr.value.mac, mac.bytes(), sizeof(macAttr.value.mac));

    // set action attribute
    sai_status_t saiRetVal = saiNeighborApi_->set_neighbor_attribute(&neighborEntry_,
                                                                      &macAttr);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
      throw SaiError("Could not set MAC: ", mac,
                     "to SAI neighbor with IP addr: ", ip_,
                     " router interface ID: ", intf_,
                     " Error: ", saiRetVal);
    }

    mac_ = mac;
  }

  if (action != action_) {

    // fill neighbor packet action attribute
    sai_attribute_t actionAttr {0};
    actionAttr.id = SAI_NEIGHBOR_ATTR_PACKET_ACTION;
    actionAttr.value.u32 = action;

    // set action attribute
    sai_status_t saiRetVal = saiNeighborApi_->set_neighbor_attribute(&neighborEntry_,
                                                                      &actionAttr);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
      throw SaiError("Could not set packet action attribute: ", action,
                     " to SAI neighbor with IP addr: ", ip_,
                     " MAC: ", mac_, " router interface ID: ", intf_,
                     " Error: ", saiRetVal);
    }

    action_ = action;
  }
}

}} // facebook::fboss
