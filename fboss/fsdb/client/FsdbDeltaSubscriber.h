// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/experimental/coro/AsyncGenerator.h>

#include <functional>

namespace facebook::fboss::fsdb {
class FsdbDeltaSubscriber : public FsdbStreamClient {
 public:
  using FsdbOperDeltaUpdateCb = std::function<void(OperDelta&&)>;
  FsdbDeltaSubscriber(
      const std::string& clientId,
      const std::vector<std::string>& subscribePath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbOperDeltaUpdateCb operDeltaUpdate,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {})
      : FsdbStreamClient(clientId, streamEvb, connRetryEvb, stateChangeCb),
        subscribePath_(subscribePath),
        operDeltaUpdate_(operDeltaUpdate) {}

  ~FsdbDeltaSubscriber() override {
    cancel();
  }

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> serviceLoop() override;
#endif
  std::vector<std::string> subscribePath_;
  FsdbOperDeltaUpdateCb operDeltaUpdate_;
};
} // namespace facebook::fboss::fsdb
