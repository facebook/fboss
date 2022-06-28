// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbComponentSyncer.h"

namespace facebook::fboss::fsdb {

void FsdbComponentSyncer::publishDelta(
    OperDelta&& /* data */,
    bool initialSync) {
  // publish on initial sync even if not ready yet
  if (!initialSync && !isReady()) {
    return;
  }
  // TODO: publish
}

void FsdbComponentSyncer::publishPath(
    OperState&& /* data */,
    bool initialSync) {
  // publish on initial sync even if not ready yet
  if (!initialSync && !isReady()) {
    return;
  }
  // TODO: publish
}
} // namespace facebook::fboss::fsdb
