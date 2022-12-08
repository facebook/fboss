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

bcm_udf_layer_t BcmUdfGroup::convertBaseHeaderToBcmLayer(
    cfg::UdfBaseHeaderType layer) {
  switch (layer) {
    case cfg::UdfBaseHeaderType::UDF_L2_HEADER:
      return bcmUdfLayerL2Header;
    case cfg::UdfBaseHeaderType::UDF_L3_HEADER:
      return bcmUdfLayerL3OuterHeader;
    case cfg::UdfBaseHeaderType::UDF_L4_HEADER:
      return bcmUdfLayerL4OuterHeader;
  }
  throw FbossError("Invalid udf base header: ", layer);
}

BcmUdfGroup::BcmUdfGroup(
    BcmSwitch* hw,
    const std::shared_ptr<UdfGroup>& udfGroup)
    : hw_(hw) {
  udfGroupName_ = udfGroup->getName();
  bcm_udf_t udfInfo;
  bcm_udf_t_init(&udfInfo);

  matchFieldWidth_ = udfGroup->getFieldSizeInBytes();
  udfInfo.layer = convertBaseHeaderToBcmLayer(udfGroup->getUdfBaseHeader());
  udfInfo.start = udfGroup->getStartOffsetInBytes();
  udfInfo.width = matchFieldWidth_;
  udfCreate(&udfInfo);
}

BcmUdfGroup::~BcmUdfGroup() {
  XLOG(DBG2) << "Destroying BcmUdfGroup";

  // Detach the udfPacketMatchedIds associated with the UdfGroup
  for (auto packetMatcherId : udfPacketMatcherIds_) {
    udfPacketMatcherDelete(packetMatcherId.first, packetMatcherId.second);
  }

  // Delete the udfGroup
  udfDelete(udfId_);
}

int BcmUdfGroup::udfCreate(bcm_udf_t* udfInfo) {
  bcm_udf_alloc_hints_t hints;
  int rv = 0;
  bcm_udf_alloc_hints_t_init(&hints);
  hints.flags = BCM_UDF_CREATE_O_UDFHASH;

  rv = bcm_udf_create(hw_->getUnit(), &hints, udfInfo, &udfId_);
  bcmCheckError(
      rv, "bcm_udf_create failed for udf group ", udfGroupName_.c_str());
  return rv;
}

int BcmUdfGroup::udfDelete(bcm_udf_id_t udfId) {
  int rv = 0;
  /* Delete udf */
  rv = bcm_udf_destroy(hw_->getUnit(), udfId);
  bcmLogFatal(
      rv, hw_, "bcm_udf_destroy failed for udf group ", udfGroupName_.c_str());
  return rv;
}

int BcmUdfGroup::udfPacketMatcherAdd(
    bcm_udf_pkt_format_id_t packetMatcherId,
    const std::string& udfPacketMatcherName) {
  int rv = 0;
  /*Attach both UDF id  to packet matcher id */
  rv = bcm_udf_pkt_format_add(hw_->getUnit(), udfId_, packetMatcherId);
  bcmCheckError(
      rv,
      "bcm_udf_pkt_format_add failed for udf group ",
      udfGroupName_.c_str(),
      ", udf id ",
      udfId_,
      " packet matcher ",
      udfPacketMatcherName.c_str(),
      " id ",
      packetMatcherId);
  udfPacketMatcherIds_.insert({packetMatcherId, udfPacketMatcherName});
  return rv;
}

int BcmUdfGroup::udfPacketMatcherDelete(
    bcm_udf_pkt_format_id_t packetMatcherId,
    const std::string& udfPacketMatcherName) {
  int rv = 0;
  /* AAttach both UDF id  to packet matcher id */
  rv = bcm_udf_pkt_format_delete(hw_->getUnit(), udfId_, packetMatcherId);
  bcmLogFatal(
      rv,
      hw_,
      "bcm_udf_pkt_format_delete() failed to detach for udf group ",
      udfGroupName_.c_str(),
      " udf id ",
      udfId_,
      " packet matcher ",
      udfPacketMatcherName.c_str(),
      " id ",
      packetMatcherId);
  auto itr = udfPacketMatcherIds_.find(packetMatcherId);
  if (itr != udfPacketMatcherIds_.end()) {
    udfPacketMatcherIds_.erase(itr);
  }
  return rv;
}

} // namespace facebook::fboss
