/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/weutil/Flags.h"

DEFINE_bool(json, false, "Output in JSON format");
DEFINE_string(
    eeprom,
    "",
    "EEPROM name or device type, default is chassis eeprom");
DEFINE_bool(h, false, "help");
DEFINE_string(
    config_file,
    "",
    "Optional platform weutil configuration file. "
    "If empty we pick the platform default config");
DEFINE_string(
    path,
    "",
    "When set, ignore config and read the eeprom specified by the path");
