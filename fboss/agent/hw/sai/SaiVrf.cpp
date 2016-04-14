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
#include "SaiVrf.h"
#include "SaiSwitch.h"
#include "SaiError.h"

namespace facebook { namespace fboss {

SaiVrf::SaiVrf(const SaiSwitch *hw, RouterID fbossVrfId)
  : hw_(hw),
    fbossVrfId_(fbossVrfId) {

  VLOG(4) << "Entering " << __FUNCTION__;

  saiVirtualRouterApi_ = hw->getSaiVrfApi();
}

SaiVrf::~SaiVrf() {
  VLOG(4) << "Entering " << __FUNCTION__;

  if (saiVrfId_ != SAI_NULL_OBJECT_ID) {
    saiVirtualRouterApi_->remove_virtual_router(saiVrfId_);
  }
}

sai_object_id_t SaiVrf::getSaiVrfId() const {

  if (saiVrfId_ == SAI_NULL_OBJECT_ID) {
    throw SaiError("Attempt to get SAI ID of the VRF ", fbossVrfId_, 
                     "which is not programmed on hardware");
  }

  return saiVrfId_;
}

void SaiVrf::program() {

  VLOG(4) << "Entering " << __FUNCTION__;

  if (saiVrfId_ == SAI_NULL_OBJECT_ID) {

    sai_status_t saiRetVal = SAI_STATUS_FAILURE;
    sai_object_id_t vrf_id = SAI_NULL_OBJECT_ID;

    sai_attribute_t attr_list[SAI_VRF_ATTR_COUNT] = {0};

    // V4 router admin state
    attr_list[0].id = SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V4_STATE;
    attr_list[0].value.booldata = true;

    // V6 router admin state
    attr_list[1].id = SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V6_STATE;
    attr_list[1].value.booldata = true;

    VLOG(2) << "Create virtual router " << fbossVrfId_.t << " through SAI";

    saiRetVal = saiVirtualRouterApi_->create_virtual_router(&vrf_id, SAI_VRF_ATTR_COUNT, attr_list);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
        throw SaiError("SAI create_virtual_router() failed : retVal: ", saiRetVal);
    }

    saiVrfId_ = vrf_id;
  }
}

}} // facebook::fboss
