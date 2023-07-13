/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/fw_util/Flags.h"

DEFINE_string(
    config_file,
    "",
    "Optional platform fw_util configuration file. "
    "If empty we pick the platform default config");

DEFINE_string(fw_target_name, "", "The fpd name that needs to be programmed");
DEFINE_string(
    fw_action,
    "",
    "The firmware action (program, verify, read, version, list) that must be taken for a specific fpd");
DEFINE_string(
    fw_binary_file,
    "",
    "The binary file that needs to be programmed to the fpd");
