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
#include "SaiHostTable.h"

namespace facebook { namespace fboss {

SaiNextHop::SaiNextHop(const SaiSwitch *hw, const RouteForwardInfo &fwdInfo)
  : hw_(hw),
    fwdInfo_(fwdInfo) {

  VLOG(4) << "Entering " << __FUNCTION__;

  for(auto nh : fwdInfo_.getNexthops()) {

    if (hw_->GetHostTable()->getSaiHost(nh.intf, nh.nexthop)) {
      resolvedNextHops_.emplace(nh);
    } else {
      unresolvedNextHops_.emplace(nh);
    }
  }

  isGroup_ = (fwdInfo_.getNexthops().size() > 1);

  pSaiNextHopApi_ = hw->GetSaiNextHopApi();
  pSaiNextHopGroupApi_ = hw->GetSaiNextHopGroupApi();
}

SaiNextHop::~SaiNextHop() {
  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiRetVal = SAI_STATUS_FAILURE;

  // Remove all next hops and group(if it was created) from the HW.
  for (auto iter : hwNextHops_) {

    if (nhGroupId_ != SAI_NULL_OBJECT_ID) {

      saiRetVal = pSaiNextHopGroupApi_->remove_next_hop_from_group(nhGroupId_, 1, &iter.first);
      if (saiRetVal != SAI_STATUS_SUCCESS) {
        LOG(ERROR) << "Could not remove next hop " << iter.second.str() << " from next hop group: "
                   << nhGroupId_ << " of next hops: " << fwdInfo_.str() <<  "Error: " << saiRetVal;
      }
    }

    saiRetVal = pSaiNextHopApi_->remove_next_hop(iter.first);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
      LOG(ERROR) << "Could not remove next hop " << iter.second.str() 
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

  if (isGroup_) {
    return nhGroupId_;
  }

  auto iter = hwNextHops_.begin();
  if (iter == hwNextHops_.end()) {
    return SAI_NULL_OBJECT_ID; 
  }

  return iter->first;
}

void SaiNextHop::onResolved(InterfaceID intf, const folly::IPAddress &ip) {
  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiRetVal {SAI_STATUS_FAILURE};

  RouteForwardInfo::Nexthop nh(intf, ip);

  // Check if this next hop is in the list of unresolved ones.
  auto iter = unresolvedNextHops_.find(nh);
  if (iter == unresolvedNextHops_.end()) {
    return;
  }

  VLOG(3) << __FUNCTION__ << " Resolving NH: " << ip << "/" << intf;

  // Program the next hop on HW.
  sai_object_id_t nhId = programNh(iter->intf, iter->nexthop);
  if (nhId == SAI_NULL_OBJECT_ID) {
    return;
  }

  if (isGroup_) {
    if (nhGroupId_ != SAI_NULL_OBJECT_ID) {
      // NH Group already exists, so just add NH to it
      saiRetVal = pSaiNextHopGroupApi_->add_next_hop_to_group(nhGroupId_, 1, &nhId);
      if (saiRetVal != SAI_STATUS_SUCCESS) {
         LOG(ERROR) << "Could not create next hop group" << nh.str() << " on HW. Error: " << saiRetVal;
         pSaiNextHopApi_->remove_next_hop(nhId);
         return;
      }

    } else {
       // No group existing. Create it with the next hop just resolved.
       sai_object_id_t nhGroupId = programNhGroup(1, &nhId);
       if (nhGroupId == SAI_NULL_OBJECT_ID) {
         pSaiNextHopApi_->remove_next_hop(nhId);
         return;
       }
    }
  }

  hwNextHops_.emplace(std::make_pair(nhId, (*iter)));
  resolvedNextHops_.emplace(*iter);
  unresolvedNextHops_.erase(iter);

  resolved_ = true;
}

void SaiNextHop::Program() {

  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiRetVal {SAI_STATUS_FAILURE};

  if (fwdInfo_.getNexthops().empty()) {
    throw SaiError("Next hop list to be programmed on HW is empty");
  }

  if ((nhGroupId_ != SAI_NULL_OBJECT_ID) || !hwNextHops_.empty()) {
    // Already configured. Nothing to do more here.
    return;
  }

  // an array of object IDs passed to create_next_hop_group()
  std::vector<sai_object_id_t> nextHopIds;

  // Create all next hops and fill the hwNextHops_ map and nextHopIds array
  for (auto iter : resolvedNextHops_) {

    sai_object_id_t nhId = programNh(iter.intf, iter.nexthop);

    if (nhId != SAI_NULL_OBJECT_ID) {
      hwNextHops_.emplace(nhId, iter);
      nextHopIds.push_back(nhId);
    }
  }

  if (!isGroup_ || nextHopIds.empty()) {
    // Nothing more to do for a single next hop or a group 
    // with empty NH list.
    return;
  }

  // Add group with the Next Hops configured above on HW
  sai_object_id_t nhGroupId = programNhGroup(nextHopIds.size(), nextHopIds.data());
  if (nhGroupId == SAI_NULL_OBJECT_ID) {
    // Cleanup hosts
    for (auto iter : hwNextHops_) {

      saiRetVal = pSaiNextHopApi_->remove_next_hop(iter.first);
      if (saiRetVal != SAI_STATUS_SUCCESS) {
        LOG(ERROR) << "Could not remove next hop " << iter.second.str() 
                   << " from HW. Error: " << saiRetVal;
      }

      unresolvedNextHops_.emplace(iter.second);
    }

    resolvedNextHops_.clear();
    hwNextHops_.clear();
  }

  resolved_ = true;
}

sai_object_id_t SaiNextHop::programNh(InterfaceID intf, const folly::IPAddress &ip) {

  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiRetVal {SAI_STATUS_FAILURE};
  std::vector<sai_attribute_t> attrList;
  sai_object_id_t nhId {SAI_NULL_OBJECT_ID};
  sai_attribute_t attr;

  // Fill atributes
  // NH type
  attr.id = SAI_NEXT_HOP_ATTR_TYPE;
  attr.value.u32 = SAI_NEXT_HOP_IP;
  attrList.push_back(attr);

  // IP address
  attr.id = SAI_NEXT_HOP_ATTR_IP;

  if (ip.isV4()) {
    attr.value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    memcpy(&attr.value.ipaddr.addr.ip4, ip.bytes(), sizeof(attr.value.ip4));
  } else {
    attr.value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    memcpy(attr.value.ipaddr.addr.ip6, ip.bytes(), sizeof(attr.value.ip6));
  }
  attrList.push_back(attr);

  // Router interface ID
  attr.id = SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID;
  attr.value.oid = hw_->GetIntfTable()->GetIntfIf(intf)->GetIfId();
  attrList.push_back(attr);

  // Create NH in SAI
  saiRetVal = pSaiNextHopApi_->create_next_hop(&nhId, attrList.size(), attrList.data());
  if (saiRetVal != SAI_STATUS_SUCCESS) {
     LOG(ERROR) << "Could not create next hop" << ip << " on HW. Error: " << saiRetVal;
  }

  return nhId;
}

sai_object_id_t SaiNextHop::programNhGroup(uint32_t nhCount, sai_object_id_t *nhIds) {

  VLOG(4) << "Entering " << __FUNCTION__;

  sai_status_t saiRetVal {SAI_STATUS_FAILURE};

  if (nhGroupId_ == SAI_NULL_OBJECT_ID) {
    // Now we will create NH group with the next hops list
    std::vector<sai_attribute_t> groupAttrList;
    sai_attribute_t attr;
    sai_object_id_t nhGroupId = SAI_NULL_OBJECT_ID;
    sai_object_list_t nhList = {nhCount, nhIds};

    // Fill atributes
    // NH group type
    attr.id = SAI_NEXT_HOP_GROUP_ATTR_TYPE;
    attr.value.u32 = SAI_NEXT_HOP_GROUP_ECMP;
    groupAttrList.push_back(attr);

    // NH list
    attr.id = SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_LIST;
    attr.value.objlist = nhList;
    groupAttrList.push_back(attr);

    // Create NH group in SAI
    saiRetVal = pSaiNextHopGroupApi_->create_next_hop_group(&nhGroupId, groupAttrList.size(), groupAttrList.data());
    if (saiRetVal != SAI_STATUS_SUCCESS) {
       LOG(ERROR) << "Could not create next hop group" << fwdInfo_.str() << " on HW. Error: " << saiRetVal;
       return SAI_NULL_OBJECT_ID;
    }

    nhGroupId_ = nhGroupId;

  } else {
    // Add NH to group in SAI
    saiRetVal = pSaiNextHopGroupApi_->add_next_hop_to_group(nhGroupId_, nhCount, nhIds);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
       LOG(ERROR) << "Could not add next hop to group " << fwdInfo_.str() << " on HW. Error: " << saiRetVal;
       return SAI_NULL_OBJECT_ID;
    }
  }

  return nhGroupId_;
}

}} // facebook::fboss
