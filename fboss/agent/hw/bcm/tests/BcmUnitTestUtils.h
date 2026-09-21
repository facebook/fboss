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

extern "C" {
/*
 * Linking with BCM libs requires us to define these symbols in our
 * application. For actual wedge_agent code we define these in
 * BcmFacebookAPI.cpp. For test functions, rather than pulling in
 * all BCM api code just define these symbols here and have tests link
 */

int bde_create();
void sal_config_init_defaults();
}
