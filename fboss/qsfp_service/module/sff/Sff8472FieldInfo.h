/* * Copyright (c) 2004-present, Facebook, Inc.  * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once
#include <cstdint>
#include <map>

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

/*
 * Parse transceiver data fields, as outlined in various documents
 * by the Small Form Factor (SFF) committee
 */

namespace facebook {
namespace fboss {

enum class Sff8472Field {
  IDENTIFIER, // Type of Transceiver
  ETHERNET_10G_COMPLIANCE_CODE, // 10G Ethernet Compliance codes

  ALARM_WARNING_THRESHOLDS,
};

class Sff8472FieldInfo {
 public:
  uint8_t transceiverI2CAddress;
  int dataAddress;
  std::uint32_t offset;
  std::uint32_t length;

  using Sff8472FieldMap = std::map<Sff8472Field, Sff8472FieldInfo>;

  static Sff8472FieldInfo getSff8472FieldAddress(
      const Sff8472FieldMap& map,
      Sff8472Field field);
};

} // namespace fboss
} // namespace facebook
