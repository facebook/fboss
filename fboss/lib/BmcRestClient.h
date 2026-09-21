/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/RestClient.h"

namespace facebook::fboss {
class BmcRestClient : public RestClient {
 public:
  BmcRestClient(void);
  /*
   * Endpoints for BMC Rest api
   */
  bool resetCP2112();
};

} // namespace facebook::fboss
