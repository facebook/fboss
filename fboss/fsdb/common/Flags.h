// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <gflags/gflags.h>

DECLARE_int32(fsdbPort);
DECLARE_int32(migrated_fsdbPort);
DECLARE_bool(publish_stats_to_fsdb);
DECLARE_bool(publish_state_to_fsdb);
DECLARE_bool(subscribe_to_stats_from_fsdb);
