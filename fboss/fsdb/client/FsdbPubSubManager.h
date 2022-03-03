// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once
#include <folly/Synchronized.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <string>
#include <vector>

namespace facebook::fboss::fsdb {
class FsdbDeltaPublisher;
class FsdbPubSubManager {
 public:
  explicit FsdbPubSubManager(const std::string& clientId);
  ~FsdbPubSubManager();

  void createDeltaPublisher(
      const std::vector<std::string>& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);
  void publish(const OperDelta& pubUnit);
  void addSubscription(
      const std::vector<std::string>& subscribePath,
      FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      const std::string& fsdbHost = "::1",
      int32_t fsdbPort = FLAGS_fsdbPort);

  void removeSubscription(
      const std::vector<std::string>& subscribePath,
      const std::string& fsdbHost = "::1");

 private:
  void stopDeltaPublisher(std::unique_ptr<FsdbDeltaPublisher>& deltaPublisher);

  folly::ScopedEventBaseThread reconnectThread_;
  folly::ScopedEventBaseThread publisherStreamEvbThread_;
  folly::ScopedEventBaseThread subscribersStreamEvbThread_;
  const std::string clientId_;
  // TODO - support multiple publishers?
  folly::Synchronized<std::unique_ptr<FsdbDeltaPublisher>> deltaPublisher_;
  folly::Synchronized<
      std::unordered_map<std::string, std::unique_ptr<FsdbDeltaSubscriber>>>
      path2Subscriber_;
};
} // namespace facebook::fboss::fsdb
