/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/sensor_service/Flags.h"

DEFINE_uint32(
    sensor_fetch_interval,
    5,
    "The interval between each sensor data fetch");

DEFINE_int32(
    stats_publish_interval,
    60,
    "Interval (in seconds) for publishing stats");

DEFINE_int32(thrift_port, 5970, "Port for the thrift service");

DEFINE_string(
    config_file,
    "",
    "Optional platform sensor configuration file. "
    "If empty we pick the platform default config");
