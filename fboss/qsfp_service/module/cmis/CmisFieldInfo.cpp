/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "CmisFieldInfo.h"
#include "fboss/agent/FbossError.h"

/*
 * Parse transceiver data fields, as outlined in
 * Common Management Interface Specification by SFF commitee at
 * http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf
 */

namespace facebook {
namespace fboss {

double CmisFieldInfo::getTemp(const uint16_t temp) {
  double data;
  data = temp / 256.0;
  if (data > 128) {
    data = data - 256;
  }
  return data;
}

double CmisFieldInfo::getVcc(const uint16_t temp) {
  double data;
  data = temp / 10000.0;
  return data;
}

double CmisFieldInfo::getTxBias(const uint16_t temp) {
  double data;
  data = temp * 2.0 / 1000;
  return data;
}

double CmisFieldInfo::getPwr(const uint16_t temp) {
  double data;
  data = temp * 0.1 / 1000;
  return data;
}

double CmisFieldInfo::getSnr(const uint16_t data) {
  double snr;
  snr = data / 256.0;
  return snr;
}

FeatureState CmisFieldInfo::getFeatureState(
    const uint8_t support,
    const uint8_t enabled) {
  if (!support) {
    return FeatureState::UNSUPPORTED;
  }
  return enabled ? FeatureState::ENABLED : FeatureState::DISABLED;
}

CmisFieldInfo CmisFieldInfo::getCmisFieldAddress(
    const CmisFieldMap& map,
    const CmisField field) {
  auto info = map.find(field);
  if (info == map.end()) {
    throw FbossError("Invalid CMIS Field ID");
  }
  return info->second;
}

} // namespace fboss
} // namespace facebook
