/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

extern "C" {
#include <bcm/error.h>
}

#include <folly/logging/xlog.h>

/*
 * bde_create() must be defined as a symbol when linking against BRCM libs.
 * It should never be invoked in our setup though. So return a error
 */
extern "C" int bde_create() {
  XLOG(ERR) << "unexpected call to bde_create(): probe invoked "
               "via diag shell command?";
  return BCM_E_UNAVAIL;
}

/*
 * We don't set any default values.
 */
extern "C" void sal_config_init_defaults() {}
