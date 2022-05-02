// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/Format.h>
#include <folly/String.h>
#include <folly/experimental/coro/AsyncGenerator.h>

#include <functional>

namespace facebook::fboss::fsdb {
template <typename SubUnit>
class FsdbSubscriber : public FsdbStreamClient {
  std::string typeStr() {
    return std::is_same_v<SubUnit, OperDelta> ? "Delta" : "Path";
  }

 public:
  using FsdbSubUnitUpdateCb = std::function<void(SubUnit&&)>;
  FsdbSubscriber(
      const std::string& clientId,
      const std::vector<std::string>& subscribePath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbSubUnitUpdateCb operSubUnitUpdate,
      bool subscribeStats,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {})
      : FsdbStreamClient(
            clientId,
            streamEvb,
            connRetryEvb,
            folly::sformat(
                "fsdb{}{}Subscriber_{}",
                typeStr(),
                (subscribeStats ? "Stat" : "State"),
                folly::join('_', subscribePath)),
            stateChangeCb),
        operSubUnitUpdate_(operSubUnitUpdate),
        subscribePath_(subscribePath),
        subscribeStats_(subscribeStats) {}

  bool subscribeStats() const {
    return subscribeStats_;
  }

 protected:
  OperSubRequest createRequest() const;
  FsdbSubUnitUpdateCb operSubUnitUpdate_;

 private:
  const std::vector<std::string> subscribePath_;
  const bool subscribeStats_;
};
} // namespace facebook::fboss::fsdb
