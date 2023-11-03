/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/platforms/wedge/QsfpRestClient.h"
#include <folly/IPAddress.h>

using folly::IPAddress;

namespace facebook::fboss {
QsfpRestClient::QsfpRestClient(void)
    : RestClient(IPAddress("fe80::1%usb0"), 8080) {}

/*
 * postQsfpThermalData
 *
 * Post the QSFP Thermal data (serialized JSON format) to the USB0 interface on
 * the given Rest endpoint path.
 */
bool QsfpRestClient::postQsfpThermalData(const std::string& thermalData) {
  auto ret =
      RestClient::requestWithOutput("/api/sys/opticsThermal", thermalData);
  auto status = ret.find("done");
  if (status != std::string::npos) {
    return true;
  }
  return false;
}

} // namespace facebook::fboss
