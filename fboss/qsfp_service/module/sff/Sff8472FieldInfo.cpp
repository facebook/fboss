/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "fboss/qsfp_service/module/sff/Sff8472FieldInfo.h"
#include "fboss/agent/FbossError.h"

/*
 * Parse transceiver data fields, as outlined in various documents
 * by the Small Form Factor (SFF) committee
 */

namespace facebook {
namespace fboss {

Sff8472FieldInfo Sff8472FieldInfo::getSff8472FieldAddress(
    const Sff8472FieldMap& map,
    const Sff8472Field field) {
  auto info = map.find(field);
  if (info == map.end()) {
    throw FbossError("Invalid SFF8472 Field ID %d", field);
  }
  return info->second;
}

double Sff8472FieldInfo::getTemp(const uint16_t temp) {
  double data;
  data = temp / 256.0;
  if (data > 128) {
    data = data - 256;
  }
  return data;
}

double Sff8472FieldInfo::getVcc(const uint16_t vcc) {
  double data;
  data = vcc / 10000.0;
  return data;
}

double Sff8472FieldInfo::getTxBias(const uint16_t txBias) {
  double data;
  data = txBias * 2.0 / 1000;
  return data;
}

double Sff8472FieldInfo::getPwr(const uint16_t pwr) {
  double data;
  data = pwr * 0.1 / 1000;
  return data;
}

} // namespace fboss
} // namespace facebook
