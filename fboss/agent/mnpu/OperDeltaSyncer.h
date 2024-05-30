/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <memory>

#include "fboss/agent/MultiSwitchThriftHandler.h"

#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <string>

namespace facebook::fboss {

class HwSwitch;

class OperDeltaSyncer {
 public:
  OperDeltaSyncer(uint16_t serverPort, SwitchID switchId, HwSwitch* hw);
  void startOperSync();
  void stopOperSync();

 private:
  void initOperDeltaSync();
  void operSyncLoop();
  fsdb::OperDelta processFullOperDelta(
      fsdb::OperDelta& operDelta,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);

  uint16_t serverPort_;
  SwitchID switchId_;
  HwSwitch* hw_;
  std::shared_ptr<folly::ScopedEventBaseThread> operSyncEvbThread_;
  std::unique_ptr<std::thread> operSyncThread_;
  std::unique_ptr<apache::thrift::Client<multiswitch::MultiSwitchCtrl>>
      operSyncClient_{nullptr};
  std::atomic<bool> operSyncRunning_{false};
};
} // namespace facebook::fboss
