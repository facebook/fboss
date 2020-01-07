/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BmcRestClient.h"
#include <folly/IPAddress.h>

using folly::IPAddress;

namespace facebook::fboss {
BmcRestClient::BmcRestClient(void)
    : RestClient(IPAddress("fe80::1%usb0"), 8080) {}

bool BmcRestClient::resetCP2112() {
  return (RestClient::request("/api/sys/usb2i2c_reset"));
}

} // namespace facebook::fboss
