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

} // namespace facebook::fboss
