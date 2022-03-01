// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

namespace facebook::fboss::fsdb {

OperDelta FsdbDeltaSubscriber::tryGetNextFor(std::chrono::milliseconds ms) {
  OperDelta delta;
  publishedQueue_.try_dequeue_for(delta, ms);
  return delta;
}
OperDelta FsdbDeltaSubscriber::getNextBlocking() {
  OperDelta delta;
  publishedQueue_.dequeue(delta);
  return delta;
}
} // namespace facebook::fboss::fsdb
