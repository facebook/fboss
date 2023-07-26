// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/if/gen-cpp2/MultiSwitchCtrl.h"
#include "fboss/agent/if/gen-cpp2/multiswitch_ctrl_handlers.h"

namespace facebook::fboss {

class MultiSwitchThriftHandler
    : public apache::thrift::ServiceHandler<multiswitch::MultiSwitchCtrl> {
 public:
  explicit MultiSwitchThriftHandler(SwSwitch* sw) : sw_(sw) {}

#if FOLLY_HAS_COROUTINES
  folly::coro::Task<apache::thrift::SinkConsumer<fsdb::OperDelta, bool>>
  co_notifyStateUpdateResult(int64_t switchId) override;

  folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::LinkEvent, bool>>
  co_notifyLinkEvent(int64_t switchId) override;

  folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::FdbEvent, bool>>
  co_notifyFdbEvent(int64_t switchId) override;

  folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::RxPacket, bool>>
  co_notifyRxPacket(int64_t switchId) override;

  folly::coro::Task<apache::thrift::ServerStream<fsdb::OperDelta>>
  co_getStateUpdates(int64_t switchId) override;

  folly::coro::Task<apache::thrift::ServerStream<multiswitch::TxPacket>>
  co_getTxPackets(int64_t switchId) override;
#endif

 private:
  void ensureConfigured(folly::StringPiece function) const;
  L2Entry getL2Entry(L2EntryThrift thriftEntry) const;
  SwSwitch* sw_;
};
} // namespace facebook::fboss
