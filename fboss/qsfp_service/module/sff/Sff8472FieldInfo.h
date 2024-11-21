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
#include "fboss/qsfp_service/module/QsfpFieldInfo.h"
#include "fboss/qsfp_service/module/sff/gen-cpp2/sff8472_types.h"

/*
 * Parse transceiver data fields, as outlined in various documents
 * by the Small Form Factor (SFF) committee
 */

namespace facebook {
namespace fboss {

enum class Sff8472Pages;

enum FieldMasks : uint8_t {
  RX_LOS_MASK = 1 << 1,
  TX_FAULT_MASK = 1 << 2,
  TX_DISABLE_STATE_MASK = 1 << 7,
  DOM_IMPLEMENTED_MASK = 1 << 6,
};

class Sff8472FieldInfo : public QsfpFieldInfo<Sff8472Field, Sff8472Pages> {
 public:
  // Render degrees Celcius from fix-point integer value
  static double getTemp(uint16_t temp);

  // Render Vcc in volts from fixed-point value
  static double getVcc(uint16_t temp);

  // Render TxBias in mA from fixed-point value
  static double getTxBias(uint16_t temp);

  // Render power in mW from fixed-point value
  static double getPwr(uint16_t temp);
};

} // namespace fboss
} // namespace facebook
