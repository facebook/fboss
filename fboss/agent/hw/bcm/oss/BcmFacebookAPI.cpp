/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmFacebookAPI.h"

extern "C" {
#include <shared/bslext.h>
} // extern "C"

namespace facebook::fboss {

void BcmFacebookAPI::initBSL() {
  bsl_config_t bsl_config;
  bsl_config_t_init(&bsl_config);
  bsl_init(&bsl_config);
}
} // namespace facebook::fboss
