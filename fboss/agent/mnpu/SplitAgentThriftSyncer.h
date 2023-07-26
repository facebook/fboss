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

#include "fboss/agent/MultiSwitchThriftHandler.h"

namespace facebook::fboss {

class HwSwitch;

class SplitAgentThriftSyncer {
 public:
  explicit SplitAgentThriftSyncer(HwSwitch* hw, uint16_t serverPort);

 private:
  HwSwitch* hw_;
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread_;
  std::unique_ptr<apache::thrift::Client<multiswitch::MultiSwitchCtrl>>
      multiSwitchClient_;
};
} // namespace facebook::fboss
