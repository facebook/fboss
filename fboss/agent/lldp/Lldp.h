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

enum class LldpChassisIdType : uint8_t {
  RESERVED = 0,
  CHASSIS_ALIAS = 1,
  INTERFACE_ALIAS = 2,
  PORT_ALIAS = 3,
  MAC_ADDRESS = 4,
  NET_ADDRESS = 5,
  INTERFACE_NAME = 6,
  LOCALLY_ASSIGNED = 7,
};

enum class LldpPortIdType : uint8_t {
  RESERVED = 0,
  INTERFACE_ALIAS = 1,
  PORT_ALIAS = 2,
  MAC_ADDRESS = 3,
  NET_ADDRESS = 4,
  INTERFACE_NAME = 5,
  AGENT_CIRCUIT_ID = 6,
  LOCALLY_ASSIGNED = 7,
};

} // namespace facebook::fboss
