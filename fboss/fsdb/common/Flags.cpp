// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_constants.h"

DEFINE_int32(
    fsdbPort,
    facebook::fboss::fsdb::fsdb_common_constants::PORT(),
    "FSDB server port");
// current default 5908 is in conflict with VNC ports, need to
// eventually migrate to 5958
DEFINE_int32(migrated_fsdbPort, 5958, "New FSDB thrift server port migrate to");
// Only FBOSS DSF subscriber is using this fsdbPort_high_priority port
DEFINE_int32(
    fsdbPort_high_priority,
    5957,
    "FSDB thrift server port connected using high priority tos=0xc0");
DEFINE_bool(publish_stats_to_fsdb, false, "Whether to publish stats to fsdb");
DEFINE_bool(
    publish_state_to_fsdb,
    false,
    "Whether to publish switch state to fsdb");
DEFINE_bool(
    subscribe_to_stats_from_fsdb,
    false,
    "Whether to subscribe to stats from fsdb");
DEFINE_bool(
    subscribe_to_state_from_fsdb,
    false,
    "Whether to subscribe to state from fsdb");
