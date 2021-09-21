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

} // namespace fboss
} // namespace facebook
