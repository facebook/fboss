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
#include <folly/logging/xlog.h>

using folly::IPAddress;

namespace facebook::fboss {
QsfpRestClient::QsfpRestClient(void)
    : RestClient("https://[fe80::1%eth0.4088]", 443) {
  setClientCertAndKey(
      "/var/facebook/x509_identities/server.pem",
      "/var/facebook/x509_identities/server.pem");
  setVerifyHostname(false);
}

/*
 * postQsfpThermalData
 *
 * Post the QSFP Thermal data (serialized JSON format) to the eth0.4088
 * interface on the given Rest endpoint path.
 */
bool QsfpRestClient::postQsfpThermalData(const std::string& thermalData) {
  auto ret =
      RestClient::requestWithOutput("/api/sys/optics_thermal", thermalData);
  if (ret.find(R"({"status": "OK"})") != std::string::npos) {
    return true;
  }
  XLOG(ERR) << "Bad reply from BMC's /api/sys/optics_thermal: " << ret;
  return false;
}

} // namespace facebook::fboss
