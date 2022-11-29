/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmUdfPacketMatcher.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss {

BcmUdfPacketMatcher::BcmUdfPacketMatcher(
    BcmSwitch* hw,
    const std::shared_ptr<UdfPacketMatcher>& udfPacketMatcher)
    : hw_(hw) {
  bcm_udf_pkt_format_info_t pktFormat;
  bcm_udf_pkt_format_info_t_init(&pktFormat);
  pktFormat.l2 = static_cast<uint16>(udfPacketMatcher->getUdfl2PktType());
  pktFormat.vlan_tag = BCM_PKT_FORMAT_VLAN_TAG_ANY;
  pktFormat.ip_protocol =
      static_cast<uint16>(udfPacketMatcher->getUdfl4PktType());
  pktFormat.ip_protocol_mask = 0xff;
  pktFormat.outer_ip = static_cast<uint16>(udfPacketMatcher->getUdfl3PktType());
  pktFormat.inner_ip = BCM_PKT_FORMAT_IP_NONE;
  pktFormat.tunnel = BCM_PKT_FORMAT_TUNNEL_NONE;

  udfPktFormatCreate(&pktFormat);
}

BcmUdfPacketMatcher::~BcmUdfPacketMatcher() {
  XLOG(DBG2) << "Destroying BcmUdfPacketMatcher";
  udfPktFormatDelete(udfPacketMatcherId_);
}

int BcmUdfPacketMatcher::udfPktFormatCreate(
    bcm_udf_pkt_format_info_t* pktFormat) {
  int rv = 0;
  /* Gnerate packet format to match IP/UDP packet */

  rv = bcm_udf_pkt_format_create(
      hw_->getUnit(),
      BCM_UDF_PKT_FORMAT_CREATE_O_NONE,
      pktFormat,
      &udfPacketMatcherId_);

  if (BCM_FAILURE(rv)) {
    printf("Failed to create packet format, error code: %s\n", bcm_errmsg(rv));
    return rv;
  }

  return rv;
}

int BcmUdfPacketMatcher::udfPktFormatDelete(
    bcm_udf_pkt_format_id_t pktMatcherId) {
  int rv = 0;
  /* Delete the packet format */
  rv = bcm_udf_pkt_format_destroy(hw_->getUnit(), pktMatcherId);

  if (BCM_FAILURE(rv)) {
    printf("bcm_udf_pkt_format_destroy() FAILED: %s\n", bcm_errmsg(rv));
    return rv;
  }

  return rv;
}

} // namespace facebook::fboss
