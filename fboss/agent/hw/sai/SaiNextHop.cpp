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
#include "fboss/agent/state/RouteForwardInfo.h"

#include "SaiNextHop.h"
#include "SaiSwitch.h"
#include "SaiError.h"
#include "SaiIntfTable.h"
#include "SaiIntf.h"

namespace facebook { namespace fboss {

SaiNextHop::SaiNextHop(const SaiSwitch *hw, const RouteForwardInfo &fwdInfo)
  : hw_(hw),
    fwdInfo_(fwdInfo) {

  VLOG(4) << "Entering " << __FUNCTION__;

  pSaiNextHopApi_ = hw->GetSaiNextHopApi();
  pSaiNextHopGroupApi_ = hw->GetSaiNextHopGroupApi();
}

SaiNextHop::~SaiNextHop() {
  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiRetVal = SAI_STATUS_FAILURE;

  // Remove all next hops and group(if it was created) from the HW.
  for (auto iter = nextHops_.begin(); iter != nextHops_.end(); ++iter) {

    if (nhGroupId_ != SAI_NULL_OBJECT_ID) {

      saiRetVal = pSaiNextHopGroupApi_->remove_next_hop_from_group(nhGroupId_, 1, &iter->first);
      if (saiRetVal != SAI_STATUS_SUCCESS) {
        LOG(ERROR) << "Could not remove next hop " << iter->second.str() << " from next hop group: "
                   << nhGroupId_ << " of next hops: " << fwdInfo_.str() <<  "Error: " << saiRetVal;
      }
    }

    saiRetVal = pSaiNextHopApi_->remove_next_hop(iter->first);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
      LOG(ERROR) << "Could not remove next hop " << iter->second.str() 
                 << " from HW. Error: " << saiRetVal;
    }
  }

  if (nhGroupId_ != SAI_NULL_OBJECT_ID) {
    saiRetVal = pSaiNextHopGroupApi_->remove_next_hop_group(nhGroupId_);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
      LOG(ERROR) << "Could not remove next hop group: " << nhGroupId_ 
                 << " of next hops: " << fwdInfo_.str() <<  " from HW. Error: " << saiRetVal;
    }
  }
}

sai_object_id_t SaiNextHop::GetSaiNextHopId() const {

  if (nhGroupId_ != SAI_NULL_OBJECT_ID) {
    return nhGroupId_;
  }

  auto iter = nextHops_.begin();
  if (iter == nextHops_.end()) {
    throw SaiError("Attempt to get SAI ID of the next hop: ",
                     fwdInfo_, "which is not programmed on hardware");
  }

  return iter->first;
}

void SaiNextHop::Program() {

  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiRetVal = SAI_STATUS_FAILURE;
  RouteForwardInfo::Nexthops nextHops = fwdInfo_.getNexthops();

  if (nextHops.empty()) {
    throw SaiError("Next hop list to be programmed on HW is empty");
  }

  if ((nhGroupId_ != SAI_NULL_OBJECT_ID) ||
      (nextHops_.size() != 0)) {

    throw SaiError("Attempt to configure for the second time the next hop ",
                     fwdInfo_.str().c_str(), "which was already programmed in HW");
  }

  uint8_t nh_count = nextHops.size();

  // an array of object IDs passed to create_next_hop_group()
  std::unique_ptr<sai_object_id_t[]> nextHopIds(new sai_object_id_t[nh_count]);

  // Create all next hops and fill the nextHops_ map and nextHopIds array
  uint8_t i = 0;
  for (auto iter = nextHops.begin(); iter != nextHops.end(); ++iter, ++i) {

    sai_object_id_t nhId = SAI_NULL_OBJECT_ID;
    sai_attribute_t attr_list[SAI_NH_ATTR_COUNT] = {0};

    // Fill atributes
    // NH type
    attr_list[0].id = SAI_NEXT_HOP_ATTR_TYPE;
    attr_list[0].value.u32 = SAI_NEXT_HOP_IP;

    // IP address
    attr_list[1].id = SAI_NEXT_HOP_ATTR_IP;

    if (iter->nexthop.isV4()) {
      attr_list[1].value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
      memcpy(&attr_list[1].value.ipaddr.addr.ip4, iter->nexthop.bytes(), sizeof(attr_list[1].value.ip4));
    } else {
      attr_list[1].value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
      memcpy(attr_list[1].value.ipaddr.addr.ip6, iter->nexthop.bytes(), sizeof(attr_list[1].value.ip6));
    }

    // Router interface ID
    attr_list[2].id = SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID;
    attr_list[2].value.oid = hw_->GetIntfTable()->GetIntfIf(iter->intf)->GetIfId();

    // Create NH in SAI
    saiRetVal = pSaiNextHopApi_->create_next_hop(&nhId, SAI_NH_ATTR_COUNT, attr_list);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
       throw SaiError("Could not create next hop ", iter->str().c_str(), " on HW. Error: ", saiRetVal);
    }

    nextHops_.emplace(nhId, *iter);
    nextHopIds[i] = nhId;
  }

  if (nh_count == 1) {
    // Nothing more to do for a single host.
    return;
  }

  // Now we will create NH group with the next hops previously created
  sai_attribute_t attr_list[SAI_NH_GROUP_ATTR_COUNT] = {0};
  sai_object_list_t nhList = {nh_count, nextHopIds.get()};
  sai_object_id_t nhGroupId = SAI_NULL_OBJECT_ID;

  // Fill atributes
  // NH group type
  attr_list[0].id = SAI_NEXT_HOP_GROUP_ATTR_TYPE;
  attr_list[0].value.u32 = SAI_NEXT_HOP_GROUP_ECMP;

  // NH list
  attr_list[1].id = SAI_NEXT_HOP_ATTR_IP;
  attr_list[1].value.objlist = nhList;

  // Create NH group in SAI
  saiRetVal = pSaiNextHopGroupApi_->create_next_hop_group(&nhGroupId, SAI_NH_GROUP_ATTR_COUNT, attr_list);
  if (saiRetVal != SAI_STATUS_SUCCESS) {
     throw SaiError("Could not create next hop group", fwdInfo_.str().c_str(), " on HW. Error: ", saiRetVal);
  }

  nhGroupId_ = nhGroupId;
}

}} // facebook::fboss
