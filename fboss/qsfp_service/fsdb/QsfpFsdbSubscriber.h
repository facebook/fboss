// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStateSubManager.h"
#include "fboss/qsfp_service/PortManager.h"
#include "fboss/qsfp_service/TransceiverManager.h"

DECLARE_bool(enable_fsdb_patch_subscriber);

namespace facebook::fboss {

/*
 * QsfpFsdbSubscriber class:
 *
 * This class subscribes to switch state from FSDB for port status
 */
class QsfpFsdbSubscriber {
 public:
  QsfpFsdbSubscriber();

  void subscribeToSwitchStatePortMap(
      TransceiverManager* tcvrManager,
      PortManager* portManager);

  void stop();

 private:
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::unique_ptr<fsdb::FsdbCowStateSubManager> fsdbSubMgr_;
};

} // namespace facebook::fboss
