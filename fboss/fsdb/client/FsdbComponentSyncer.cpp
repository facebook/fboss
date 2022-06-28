// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbComponentSyncer.h"
#include "fboss/fsdb/client/FsdbSyncManager.h"

namespace facebook::fboss::fsdb {

void FsdbComponentSyncer::publishDelta(OperDelta&& data, bool initialSync) {
  // publish on initial sync even if not ready yet
  if (!initialSync && !isReady()) {
    return;
  }
  syncManager_->publishState(std::move(data));
}

void FsdbComponentSyncer::publishPath(OperState&& data, bool initialSync) {
  // publish on initial sync even if not ready yet
  if (!initialSync && !isReady()) {
    return;
  }
  syncManager_->publishState(std::move(data));
}
} // namespace facebook::fboss::fsdb
