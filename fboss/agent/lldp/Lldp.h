// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/agent/types.h"

namespace facebook::fboss {

enum class LldpTlvType : uint16_t {
  PDU_END = 0,
  CHASSIS = 1,
  PORT = 2,
  TTL = 3,
  PORT_DESC = 4,
  SYSTEM_NAME = 5,
  SYSTEM_DESCRIPTION = 6,
  SYSTEM_CAPABILITY = 7,
  MANAGEMENT_ADDRESS = 8,
  ORG_SPECIFIC = 127,
};

// Facebook-specific TLV subtypes
enum class FacebookLldpSubtype : uint8_t {
  PORT_DRAIN_STATE = 1,
};

// Port drain state TLV length: OUI (3) + Subtype (1) + Value (1) = 5
constexpr uint16_t PORT_DRAIN_STATE_TLV_LENGTH = 0x5;

} // namespace facebook::fboss
