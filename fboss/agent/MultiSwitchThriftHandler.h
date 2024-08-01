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
  void cancelEventSyncers();

#if FOLLY_HAS_COROUTINES
  folly::coro::Task<
      apache::thrift::SinkConsumer<multiswitch::LinkChangeEvent, bool>>
  co_notifyLinkChangeEvent(int64_t switchId) override;
  folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::FdbEvent, bool>>
  co_notifyFdbEvent(int64_t switchId) override;

  folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::RxPacket, bool>>
  co_notifyRxPacket(int64_t switchId) override;

  folly::coro::Task<apache::thrift::ServerStream<multiswitch::TxPacket>>
  co_getTxPackets(int64_t switchId) override;

  folly::coro::Task<
      apache::thrift::SinkConsumer<multiswitch::HwSwitchStats, bool>>
  co_syncHwStats(int16_t switchIndex) override;

  folly::coro::Task<apache::thrift::SinkConsumer<
      multiswitch::SwitchReachabilityChangeEvent,
      bool>>
  co_notifySwitchReachabilityChangeEvent(int64_t switchIndex) override;
#endif
  void getNextStateOperDelta(
      multiswitch::StateOperDelta& operDelta,
      int64_t switchId,
      std::unique_ptr<multiswitch::StateOperDelta> prevOperResult,
      int64_t lastUpdateSeqNum) override;

  void gracefulExit(int64_t switchId) override;

  static L2Entry getL2Entry(L2EntryThrift thriftEntry);

 private:
  void processLinkState(
      SwitchID switchId,
      const multiswitch::LinkChangeEvent& linkChangeEvent);
  void processLinkActiveState(
      SwitchID switchId,
      const multiswitch::LinkChangeEvent& linkChangeEvent);
  void processLinkConnectivity(
      SwitchID switchId,
      const multiswitch::LinkChangeEvent& linkChangeEvent);
  void processSwitchReachabilityChangeEvent(
      SwitchID switchId,
      const multiswitch::SwitchReachabilityChangeEvent&
          switchReachabilityChangeEvent);
  void ensureConfigured(folly::StringPiece function) const;
  SwSwitch* sw_;
  folly::CancellationSource rxPktCancellationSource_;
  folly::CancellationSource linkCancellationSource_;
  folly::CancellationSource fdbCancellationSource_;
  folly::CancellationSource statsCancellationSource_;
  folly::CancellationSource switchReachabilityCancellationSource_;
};
} // namespace facebook::fboss
