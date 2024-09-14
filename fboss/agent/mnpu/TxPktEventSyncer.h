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
#include "fboss/agent/mnpu/SplitAgentThriftSyncerClient.h"

#include <folly/io/async/EventBase.h>
#include <string>

namespace facebook::fboss {

class HwSwitch;

class TxPktEventSyncer : public ThriftStreamClient<multiswitch::TxPacket> {
 public:
  TxPktEventSyncer(
      uint16_t serverPort,
      SwitchID switchId,
      folly::EventBase* connRetryEvb,
      HwSwitch* hw,
      std::optional<std::string> multiSwitchStatsPrefix);

#if FOLLY_HAS_COROUTINES
  static ThriftStreamClient<multiswitch::TxPacket>::EventNotifierStreamClient
  initTxPktEventStream(
      SwitchID switchId,
      apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client);
#endif
  static fb303::TimeseriesWrapper& getBadPacketCounter();
  static void TxPacketEventHandler(multiswitch::TxPacket&, HwSwitch*);

 private:
  void connected() override {}
  static apache::thrift::RpcOptions getRpcOptions();
};
} // namespace facebook::fboss
