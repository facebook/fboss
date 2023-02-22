/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmUnitTestUtils.h"

#include <assert.h>

extern "C" {
int bde_create() {
  assert(0);
  return 0;
}
void sal_config_init_defaults() {
  assert(0);
}
}
