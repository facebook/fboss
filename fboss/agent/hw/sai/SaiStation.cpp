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
#include "SaiStation.h"
#include "SaiSwitch.h"

extern "C" {
#include "saiacl.h"
}

namespace facebook { namespace fboss {

SaiStation::SaiStation(const SaiSwitch *hw)
  : hw_(hw) {
  VLOG(4) << "Entering " << __FUNCTION__;
}

SaiStation::~SaiStation() {
  VLOG(4) << "Entering " << __FUNCTION__;

  if(id_ == INVALID) {
    return;
  }

  hw_->GetSaiAclApi()->delete_acl_table(id_);

  VLOG(3) << "deleted SAI station entry " << id_;
}

void SaiStation::Program(folly::MacAddress mac, sai_object_id_t aclTableId) {
  VLOG(4) << "Entering " << __FUNCTION__;

  CHECK_EQ(INVALID, id_);
  
  sai_object_id_t tableId = aclTableId;
  sai_attribute_t attrList[3];

  attrList[0].id = SAI_ACL_TABLE_ATTR_STAGE;
  attrList[0].value.u32 = 0; 

  attrList[1].id = SAI_ACL_TABLE_ATTR_FIELD_DST_MAC;
  memcpy(&attrList[1].value.mac, mac.bytes(), sizeof(sai_mac_t));

  attrList[2].id = SAI_ACL_TABLE_ATTR_FIELD_IP_TYPE;
  attrList[2].value.u32 = SAI_ACL_IP_TYPE_ANY;

  hw_->GetSaiAclApi()->create_acl_table(&tableId, 3, attrList);
  id_ = aclTableId;
  
  VLOG (1) << "Adding SAI station with Mac : " << mac <<" and " << aclTableId;

  CHECK_NE(id_, INVALID);
}

}} // facebook::fboss
