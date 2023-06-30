// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/Format.h>
#include <folly/String.h>
#include <folly/experimental/coro/AsyncGenerator.h>

#include <functional>

namespace facebook::fboss::fsdb {

std::string extendedPathsStr(const std::vector<ExtendedOperPath>& path);

template <typename SubUnit, typename PathElement>
class FsdbSubscriber : public FsdbStreamClient {
  using Paths = std::vector<PathElement>;
  std::string typeStr() const;
  std::string pathsStr(const Paths& path) const;

 public:
  using FsdbSubUnitUpdateCb = std::function<void(SubUnit&&)>;
  using SubUnitT = SubUnit;
  FsdbSubscriber(
      const std::string& clientId,
      const Paths& subscribePaths,
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
                pathsStr(subscribePaths)),
            subscribeStats,
            stateChangeCb),
        operSubUnitUpdate_(operSubUnitUpdate),
        subscribePaths_(subscribePaths) {}

 protected:
  auto createRequest() const {
    if constexpr (std::is_same_v<PathElement, std::string>) {
      OperPath operPath;
      operPath.raw() = subscribePaths_;
      OperSubRequest request;
      request.path() = operPath;
      request.subscriberId() = clientId();
      return request;
    } else {
      OperSubRequestExtended request;
      request.paths() = subscribePaths_;
      request.subscriberId() = clientId();
      return request;
    }
  }
  FsdbSubUnitUpdateCb operSubUnitUpdate_;

 private:
  const Paths subscribePaths_;
};
} // namespace facebook::fboss::fsdb
