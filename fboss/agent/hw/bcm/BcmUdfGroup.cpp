/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmUdfGroup.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss {

BcmUdfGroup::BcmUdfGroup(
    BcmSwitch* hw,
    const std::shared_ptr<UdfGroup>& udfGroup)
    : hw_(hw) {
  udfGroupName_ = udfGroup->getName();
  bcm_udf_t udfInfo;
  bcm_udf_t_init(&udfInfo);
  udfInfo.layer = static_cast<bcm_udf_layer_t>(udfGroup->getUdfBaseHeader());
  udfInfo.start = udfGroup->getStartOffsetInBytes();
  udfInfo.width = udfGroup->getFieldSizeInBytes();

  udfCreate(&udfInfo);
}

BcmUdfGroup::~BcmUdfGroup() {
  XLOG(DBG2) << "Destroying BcmUdfGroup";
  udfDelete(udfId_);
}

int BcmUdfGroup::udfCreate(bcm_udf_t* udfInfo) {
  bcm_udf_alloc_hints_t hints;
  int rv = 0;
  bcm_udf_alloc_hints_t_init(&hints);
  hints.flags = BCM_UDF_CREATE_O_UDFHASH;

  rv = bcm_udf_create(hw_->getUnit(), &hints, udfInfo, &udfId_);
  if (BCM_FAILURE(rv)) {
    printf(
        "bcm_udf_create() failed for %s, error_code : %s\n",
        udfGroupName_.c_str(),
        bcm_errmsg(rv));
    return rv;
  }

  return rv;
}

int BcmUdfGroup::udfDelete(bcm_udf_id_t udfId) {
  int rv = 0;
  /* Delete udf */
  rv = bcm_udf_destroy(hw_->getUnit(), udfId);

  if (BCM_FAILURE(rv)) {
    printf(
        "bcm_udf_destroy() failed for %s, error_code: %s\n",
        udfGroupName_.c_str(),
        bcm_errmsg(rv));
    return rv;
  }

  return rv;
}

} // namespace facebook::fboss
