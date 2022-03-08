// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaPublisher.h"

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

namespace facebook::fboss::fsdb {

void FsdbDeltaPublisher::write(const OperDelta& pubUnit) {
  if (!toPublishQueue_.try_enqueue(pubUnit)) {
    FsdbException ex;
    ex.errorCode_ref() = FsdbErrorCode::DROPPED;
    ex.message_ref() = "Unable to queue delta";
    throw ex;
  }
}
#if FOLLY_HAS_COROUTINES
folly::coro::AsyncGenerator<OperDelta> FsdbDeltaPublisher::createGenerator() {
  while (true) {
    OperDelta delta;
    toPublishQueue_.try_dequeue_for(delta, std::chrono::milliseconds(10));
    co_yield delta;
  }
}
#endif
} // namespace facebook::fboss::fsdb
