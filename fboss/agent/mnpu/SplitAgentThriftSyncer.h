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

#include <folly/io/async/ScopedEventBaseThread.h>
#include <gflags/gflags.h>
#include <string>

#include "fboss/agent/HwSwitchCallback.h"
#include "fboss/agent/MultiSwitchThriftHandler.h"

namespace facebook::fboss {

class HwSwitch;

class SplitAgentThriftSyncer : public HwSwitchCallback {
 public:
  SplitAgentThriftSyncer(HwSwitch* hw, uint16_t serverPort);

  void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept override;
  void linkStateChanged(
      PortID port,
      bool up,
      std::optional<phy::LinkFaultStatus> iPhyFaultStatus =
          std::nullopt) override;
  void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) override;
  void exitFatal() const noexcept override;
  void pfcWatchdogStateChanged(const PortID& port, const bool deadlock)
      override;
  void registerStateObserver(StateObserver* observer, const std::string& name)
      override;
  void unregisterStateObserver(StateObserver* observer) override;

  void connect();

 private:
  HwSwitch* hw_;
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread_;
  std::unique_ptr<apache::thrift::Client<multiswitch::MultiSwitchCtrl>>
      multiSwitchClient_;
  uint16_t serverPort_{0};
};
} // namespace facebook::fboss
