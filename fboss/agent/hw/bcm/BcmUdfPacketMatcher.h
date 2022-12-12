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
#include "fboss/agent/state/UdfPacketMatcher.h"

namespace facebook::fboss {

class BcmSwitch;

/**
 * BcmUdfPacketMatcher is the class to abstract bcm_udf_pkt_format_create in
 * BcmSwitch

 */
class BcmUdfPacketMatcher {
 public:
  BcmUdfPacketMatcher(
      BcmSwitch* hw,
      const std::shared_ptr<UdfPacketMatcher>& udfPacketMatcher);
  ~BcmUdfPacketMatcher();

  bcm_udf_pkt_format_id_t getUdfPacketMatcherId() const {
    return udfPacketMatcherId_;
  }

 private:
  int udfPktFormatCreate(bcm_udf_pkt_format_info_t* pktFormat);
  int udfPktFormatDelete(bcm_udf_pkt_format_id_t pktMatcherId);

  int convertUdfL2PktTypeToBcmType(cfg::UdfMatchL2Type l2Type);
  int convertUdfl4TypeToIpProtocol(cfg::UdfMatchL4Type l4Type);
  int convertUdfL3PktTypeToBcmType(cfg::UdfMatchL3Type l3Type);
  bool isBcmPktFormatCacheMatchesCfg(
      const bcm_udf_pkt_format_info_t* cachedPktFormat,
      const bcm_udf_pkt_format_info_t* pktFormat);

  BcmSwitch* hw_;
  bcm_udf_pkt_format_id_t udfPacketMatcherId_ = 0;
};

} // namespace facebook::fboss
