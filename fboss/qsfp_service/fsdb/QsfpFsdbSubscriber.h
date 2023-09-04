// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/qsfp_service/TransceiverManager.h"

#include <memory>

namespace facebook::fboss {
namespace fsdb {
class FsdbPubSubManager;
}

/*
 * QsfpFsdbSubscriber class:
 *
 * This class subscribes to switch state from FSDB for port status
 */
class QsfpFsdbSubscriber {
 public:
  explicit QsfpFsdbSubscriber(fsdb::FsdbPubSubManager* pubSubMgr)
      : fsdbPubSubMgr_(pubSubMgr) {}

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_;
  }

  void subscribeToSwitchStatePortMap(TransceiverManager* tcvrManager);
  void removeSwitchStatePortMapSubscription();

  static std::vector<std::string> getSwitchStatePortMapPath();

 private:
  fsdb::FsdbPubSubManager* fsdbPubSubMgr_;
};

} // namespace facebook::fboss
