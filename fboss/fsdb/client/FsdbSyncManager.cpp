// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbSyncManager.h"

DEFINE_bool(
    publish_use_id_paths,
    false,
    "Publish paths using thrift ids rather than names");

DEFINE_bool(
    fsdb_publish_state_with_patches,
    false,
    "Publish state to FSDB using Patches instead of Deltas, default false");

namespace facebook::fboss::fsdb {

PubSubType getFsdbStatePubType() {
  return FLAGS_fsdb_publish_state_with_patches ? PubSubType::PATCH
                                               : PubSubType::DELTA;
}

} // namespace facebook::fboss::fsdb
