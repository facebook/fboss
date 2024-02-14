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
#include <string>

#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwSwitchThriftClientTable {
 public:
  HwSwitchThriftClientTable(
      int16_t basePort,
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);
  apache::thrift::Client<FbossHwCtrl>* getClient(SwitchID switchId);

  std::optional<std::map<::std::int64_t, FabricEndpoint>> getFabricConnectivity(
      SwitchID switchId);

 private:
  apache::thrift::Client<FbossHwCtrl> createClient(
      int16_t port,
      std::shared_ptr<folly::ScopedEventBaseThread> evbThread);

  std::map<
      SwitchID,
      std::pair<
          std::unique_ptr<apache::thrift::Client<FbossHwCtrl>>,
          std::shared_ptr<folly::ScopedEventBaseThread>>>
      clientInfos_;
};
} // namespace facebook::fboss
