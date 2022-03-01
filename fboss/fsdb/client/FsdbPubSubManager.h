// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once
#include <folly/io/async/ScopedEventBaseThread.h>
#include "fboss/fsdb/Flags.h"
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

 private:
  void stopDeltaPublisher();
  folly::ScopedEventBaseThread reconnectThread_;
  folly::ScopedEventBaseThread publisherStreamEvbThread_;
  const std::string clientId_;
  // TODO - support multiple publishers?
  std::unique_ptr<FsdbDeltaPublisher> deltaPublisher_;
};
} // namespace facebook::fboss::fsdb
