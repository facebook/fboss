// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_constants.h"

DEFINE_int32(
    fsdbPort,
    facebook::fboss::fsdb::fsdb_common_constants::PORT(),
    "FSDB server port");
DEFINE_bool(publish_stats_to_fsdb, false, "Whether to publish stats to fsdb");
DEFINE_bool(
    publish_state_to_fsdb,
    false,
    "Whether to publish switch state to fsdb");
