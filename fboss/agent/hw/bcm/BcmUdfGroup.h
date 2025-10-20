/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

extern "C" {
#include <bcm/types.h>
#include <bcm/udf.h>
}

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/state/UdfGroup.h"

namespace facebook::fboss {

class BcmSwitch;

/**
 * BcmUdfGroup is the class to abstract bcm_udf_create in
 * BcmSwitch
 */
class BcmUdfGroup {
 public:
  BcmUdfGroup(BcmSwitch* hw, const std::shared_ptr<UdfGroup>& udfGroup);
  ~BcmUdfGroup();
  BcmUdfGroup(const BcmUdfGroup&) = delete;
  BcmUdfGroup& operator=(const BcmUdfGroup&) = delete;
  BcmUdfGroup(BcmUdfGroup&&) = delete;
  BcmUdfGroup& operator=(BcmUdfGroup&&) = delete;

  bcm_udf_id_t getUdfId() const {
    return udfId_;
  }

  std::map<bcm_udf_pkt_format_id_t, std::string> getUdfPacketMatcherIds() {
    return udfPacketMatcherIds_;
  }

  int udfPacketMatcherAdd(
      bcm_udf_pkt_format_id_t packetMatcherId,
      const std::string& udfPacketMatcherName);
  int udfPacketMatcherDelete(
      bcm_udf_pkt_format_id_t packetMatcherId,
      const std::string& udfPacketMatcherName);
  void udfPacketMatcherIdsInsert(
      bcm_udf_pkt_format_id_t packetMatcherId,
      const std::string& udfPacketMatcherName);

  int getUdfMatchFieldWidth() {
    return matchFieldWidth_;
  }

  int getUdfProto() {
    return proto_;
  }

  cfg::UdfGroupType getUdfGroupType() {
    if (udfGroupType_.has_value()) {
      return udfGroupType_.value();
    }
    return cfg::UdfGroupType::HASH;
  }

 private:
  int udfCreate(bcm_udf_t* udfInfo);
  int udfDelete(bcm_udf_id_t udfId);
  int udfBcmFieldQsetMultiSet();
  bcm_udf_layer_t convertBaseHeaderToBcmLayer(cfg::UdfBaseHeaderType layer);
  bool isBcmUdfInfoCacheMatchesCfg(
      const bcm_udf_t* cachedUdfInfo,
      const bcm_udf_t* udfInfo);

  BcmSwitch* hw_;
  bcm_udf_id_t udfId_ = 0;
  int matchFieldWidth_ = 0;
  int proto_ = 0;
  std::string udfGroupName_;
  std::optional<cfg::UdfGroupType> udfGroupType_;
  std::map<bcm_udf_pkt_format_id_t, std::string> udfPacketMatcherIds_;
};

} // namespace facebook::fboss
