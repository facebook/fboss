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

void QsfpModule::getDACCableInfo(Cable &cable) {
  //Can update the gauge and length information for DAC Cables
  cable.dacLength = 0;
  cable.gauge = 0;
}

int QsfpModule::getQsfpDACGauge() {
  // Need special custom fields for determining this information
  return 0;
}

float QsfpModule::getQsfpDACLength(){
  // Need special custom fields for determining this information
  return 0.0;
}

}} //namespace facebook::fboss
