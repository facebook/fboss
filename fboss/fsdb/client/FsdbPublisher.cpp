// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbPublisher.h"

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

template <typename PubUnit>
OperPubRequest FsdbPublisher<PubUnit>::createRequest() const {
  OperPath operPath;
  operPath.raw_ref() = publishPath_;
  OperPubRequest request;
  request.path_ref() = operPath;
  request.publisherId_ref() = clientId();
  return request;
}

template <typename PubUnit>
void FsdbPublisher<PubUnit>::write(const PubUnit& pubUnit) {
  if (!toPublishQueue_.try_enqueue(pubUnit)) {
    FsdbException ex;
    ex.errorCode_ref() = FsdbErrorCode::DROPPED;
    ex.message_ref() = "Unable to queue delta";
    throw ex;
  }
}
#if FOLLY_HAS_COROUTINES
template <typename PubUnit>
folly::coro::AsyncGenerator<std::optional<PubUnit>>
FsdbPublisher<PubUnit>::createGenerator() {
  while (true) {
    PubUnit pubUnit;
    if (toPublishQueue_.try_dequeue_for(
            pubUnit, std::chrono::milliseconds(10))) {
      co_yield pubUnit;
    } else {
      co_yield std::nullopt;
    }
  }
}
#endif

template class FsdbPublisher<OperDelta>;
template class FsdbPublisher<OperState>;

} // namespace facebook::fboss::fsdb
