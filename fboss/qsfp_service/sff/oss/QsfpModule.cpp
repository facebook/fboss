/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/sff/QsfpModule.h"

namespace facebook { namespace fboss {

const folly::Optional<QsfpModule::LengthAndGauge>
QsfpModule::getDACCableOverride() const {
  return folly::none;
}

bool QsfpModule::validateQsfpString(const std::string& /*value*/) const {
  return true;
}

}} //namespace facebook::fboss
