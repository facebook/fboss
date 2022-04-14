// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbPublisher.h"

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <chrono>

namespace facebook::fboss::fsdb {

template <typename PubUnit>
OperPubRequest FsdbPublisher<PubUnit>::createRequest() const {
  OperPath operPath;
  operPath.raw() = publishPath_;
  OperPubRequest request;
  request.path() = operPath;
  request.publisherId() = clientId();
  return request;
}

template <typename PubUnit>
void FsdbPublisher<PubUnit>::write(PubUnit pubUnit) {
  if (!pubUnit.metadata()) {
    pubUnit.metadata() = OperMetadata{};
  }
  if (!pubUnit.metadata()->lastConfirmedAt()) {
    auto now = std::chrono::system_clock::now();
    pubUnit.metadata()->lastConfirmedAt() =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();
  }
  if (!toPublishQueue_.try_enqueue(std::move(pubUnit))) {
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
