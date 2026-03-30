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

#include <cstdint>
#include <vector>

#include <folly/io/Cursor.h>

#include "fboss/agent/packet/IpfixHeader.h"

namespace facebook::fboss::psamp {

// Broadcom XGS PSAMP MoD (Mirror-on-Drop) packet structs.
//
// These structs implement the Broadcom-specific "PSAMP Format 2" mirror
// encapsulation used on XGS platforms (e.g. Memory on Drop packets).
// The format deviates from standard IPFIX/PSAMP (RFC 5476) in several ways:
//
// - Uses a fixed hardcoded template ID (0x1234) instead of the standard IPFIX
//   template exchange mechanism (RFC 5101 Set ID / Template Record).
// - PSAMP data fields include Broadcom-specific switch/port identifiers
//   (switchId, egressModPortId) and ASIC-internal drop reason codes
//   (dropReasonIngress, dropReasonMmu) not defined in the IANA IPFIX
//   Information Element registry.
// - Additional vendor metadata: userMetaField (from EGR_MIRROR_USER_META_DATA
//   register) and cosColorProb (packed COS queue, color, probability index).
// - UDP checksum is always zero (RFC 5101 Section 10.3.2 requires a valid
//   checksum for IPFIX-over-UDP).
//
// Spec reference: https://pxl.cl/97k6s

constexpr uint16_t XGS_PSAMP_TEMPLATE_ID = 0x1234;
constexpr uint8_t XGS_PSAMP_VAR_LEN_INDICATOR = 0xFF;

struct XgsPsampTemplateHeader {
  uint16_t templateId{XGS_PSAMP_TEMPLATE_ID};
  uint16_t psampLength{};

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static XgsPsampTemplateHeader deserialize(folly::io::Cursor& cursor);
};

struct XgsPsampData {
  uint64_t observationTimeNs{};
  uint32_t switchId{};
  uint16_t egressModPortId{};
  uint16_t ingressPort{};
  uint8_t dropReasonIngress{};
  uint8_t dropReasonMmu{};
  uint16_t userMetaField{};
  uint8_t cosColorProb{};
  uint8_t varLenIndicator{XGS_PSAMP_VAR_LEN_INDICATOR};
  uint16_t packetSampledLength{};
  std::vector<uint8_t> sampledPacketData;

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static XgsPsampData deserialize(folly::io::Cursor& cursor);
};

struct XgsPsampModPacket {
  IpfixHeader ipfixHeader;
  XgsPsampTemplateHeader templateHeader;
  XgsPsampData data;

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static XgsPsampModPacket deserialize(folly::io::Cursor& cursor);
  // throws HdrParseError if ipfixHeader.length != size()
};

} // namespace facebook::fboss::psamp
