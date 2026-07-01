// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/reboot_cause_service/Flags.h"

DEFINE_int32(thrift_port, 5974, "Port for the thrift service");
DEFINE_bool(
    determine_reboot_cause,
    false,
    "Read reboot causes from all providers and determine the reboot cause");
DEFINE_bool(
    clear_reboot_causes,
    false,
    "Clear reboot causes from all providers after reading");
