// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"

#include <memory>

namespace facebook::fboss {
namespace fsdb {
class FsdbPubSubManager;
}

class LedManager;

/*
 * FsdbSwitchStateSubscriber class:
 *
 * This class subscribes to FSDB for the Port Info state path. The callback
 * function is called in FSDB callback thread and it will update the Port Info
 * map in Led Service.
 */
class FsdbSwitchStateSubscriber {
 public:
  explicit FsdbSwitchStateSubscriber(fsdb::FsdbPubSubManager* pubSubMgr)
      : fsdbPubSubMgr_(pubSubMgr) {}

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_;
  }

  void subscribeToSwitchState(LedManager* ledManager);

  static std::vector<std::string> getSwitchStatePath();

 private:
  void subscribeToState(std::vector<std::string> path, LedManager* ledManager);

  fsdb::FsdbPubSubManager* fsdbPubSubMgr_;
};

} // namespace facebook::fboss
