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
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/packet/IPProto.h"

namespace facebook::fboss {

int BcmUdfPacketMatcher::convertUdfl4TypeToIpProtocol(
    cfg::UdfMatchL4Type l4Type) {
  switch (l4Type) {
    case cfg::UdfMatchL4Type::UDF_L4_PKT_TYPE_UDP:
      return static_cast<int>(IP_PROTO::IP_PROTO_UDP);
    case cfg::UdfMatchL4Type::UDF_L4_PKT_TYPE_TCP:
      return static_cast<int>(IP_PROTO::IP_PROTO_TCP);
    default:
      break;
  }
  throw FbossError("Invalid udf l4 pkt type: ", l4Type);
}

int BcmUdfPacketMatcher::convertUdfL3PktTypeToBcmType(
    cfg::UdfMatchL3Type l3Type) {
  switch (l3Type) {
    case cfg::UdfMatchL3Type::UDF_L3_PKT_TYPE_ANY:
      return BCM_PKT_FORMAT_IP_ANY;
    case cfg::UdfMatchL3Type::UDF_L3_PKT_TYPE_IPV4:
      return BCM_PKT_FORMAT_IP4;
    case cfg::UdfMatchL3Type::UDF_L3_PKT_TYPE_IPV6:
      return BCM_PKT_FORMAT_IP6;
  }
  throw FbossError("Invalid udf l3 pkt type: ", l3Type);
}

int BcmUdfPacketMatcher::convertUdfL2PktTypeToBcmType(
    cfg::UdfMatchL2Type l2Type) {
  switch (l2Type) {
    case cfg::UdfMatchL2Type::UDF_L2_PKT_TYPE_ANY:
      return BCM_PKT_FORMAT_L2_ANY;
    case cfg::UdfMatchL2Type::UDF_L2_PKT_TYPE_ETH:
      return BCM_PKT_FORMAT_L2_ETH_II;
  }
  throw FbossError("Invalid udf l2 pkt type: ", l2Type);
}

bool BcmUdfPacketMatcher::isBcmPktFormatCacheMatchesCfg(
    const bcm_udf_pkt_format_info_t* cachedPktFormat,
    const bcm_udf_pkt_format_info_t* pktFormat) {
  if ((cachedPktFormat->l2 == pktFormat->l2) &&
      (cachedPktFormat->vlan_tag == pktFormat->vlan_tag) &&
      (cachedPktFormat->ip_protocol == pktFormat->ip_protocol) &&
      (cachedPktFormat->ip_protocol_mask == pktFormat->ip_protocol_mask) &&
      (cachedPktFormat->outer_ip == pktFormat->outer_ip) &&
      (cachedPktFormat->inner_ip == pktFormat->inner_ip) &&
      (cachedPktFormat->tunnel == pktFormat->tunnel)) {
    if (pktFormat->l4_dst_port && pktFormat->l4_dst_port_mask) {
      if ((cachedPktFormat->l4_dst_port != pktFormat->l4_dst_port) ||
          (cachedPktFormat->l4_dst_port_mask != pktFormat->l4_dst_port_mask)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

BcmUdfPacketMatcher::BcmUdfPacketMatcher(
    BcmSwitch* hw,
    const std::shared_ptr<UdfPacketMatcher>& udfPacketMatcher)
    : hw_(hw) {
  bcm_udf_pkt_format_info_t pktFormat;
  bcm_udf_pkt_format_info_t_init(&pktFormat);
  pktFormat.l2 = static_cast<uint16>(udfPacketMatcher->getUdfl2PktType());
  pktFormat.vlan_tag = BCM_PKT_FORMAT_VLAN_TAG_ANY;
  pktFormat.ip_protocol =
      convertUdfl4TypeToIpProtocol(udfPacketMatcher->getUdfl4PktType());
  pktFormat.ip_protocol_mask = 0xff;
  pktFormat.outer_ip =
      convertUdfL3PktTypeToBcmType(udfPacketMatcher->getUdfl3PktType());
  pktFormat.inner_ip = BCM_PKT_FORMAT_IP_NONE;
  pktFormat.tunnel = BCM_PKT_FORMAT_TUNNEL_NONE;
  if (auto udfL4DstPort = udfPacketMatcher->getUdfL4DstPort()) {
    pktFormat.l4_dst_port = *udfL4DstPort;
    pktFormat.l4_dst_port_mask = 0xffff; // 16 bits
  }

  auto warmBootCache = hw_->getWarmBootCache();
  auto name = udfPacketMatcher->getID();
  auto udfPacketMatcherInfoItr = warmBootCache->findUdfPktMatcherInfo(name);

  if (udfPacketMatcherInfoItr !=
      warmBootCache->UdfPktMatcherNameToInfoMapEnd()) {
    auto cachedPktFormat = udfPacketMatcherInfoItr->second.second;
    if (isBcmPktFormatCacheMatchesCfg(&cachedPktFormat, &pktFormat)) {
      udfPacketMatcherId_ = udfPacketMatcherInfoItr->second.first;
      warmBootCache->programmed(udfPacketMatcherInfoItr);
      XLOG(DBG2) << "Warmboot PktFormat cache matches the cfg for " << name;
      return;
    }
  }

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
