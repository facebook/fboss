// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/concurrency/DynamicBoundedQueue.h>
#include <folly/experimental/coro/AsyncGenerator.h>
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {
template <typename PubUnit>
class FsdbPublisher : public FsdbStreamClient {
 public:
  FsdbPublisher(
      const std::string& clientId,
      const std::vector<std::string>& publishPath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {})
      : FsdbStreamClient(clientId, streamEvb, connRetryEvb, stateChangeCb),
        publishPath_(publishPath) {}

  void write(const PubUnit& pubUnit);

  ssize_t queueSize() const {
    return toPublishQueue_.size();
  }

 protected:
#if FOLLY_HAS_COROUTINES
  folly::coro::AsyncGenerator<std::optional<PubUnit>> createGenerator();
#endif
  OperPubRequest createRequest() const;

 private:
  static constexpr auto kPubQueueCapacity{2000};
  folly::DMPSCQueue<PubUnit, true /*may block*/> toPublishQueue_{
      kPubQueueCapacity};
  const std::vector<std::string> publishPath_;
};
} // namespace facebook::fboss::fsdb
