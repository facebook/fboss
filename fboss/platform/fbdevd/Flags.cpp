/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/fbdevd/Flags.h"

DEFINE_string(
    config_file,
    "",
    "Optional platform fbdevd configuration file. "
    "If empty we pick the platform default config from config lib");
