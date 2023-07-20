// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MultiSwitchThriftHandler.h"

namespace facebook::fboss {

#if FOLLY_HAS_COROUTINES
folly::coro::Task<apache::thrift::SinkConsumer<fsdb::OperDelta, bool>>
MultiSwitchThriftHandler::co_notifyStateUpdateResult(int64_t /*switchId*/) {
  co_return {};
}

folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::LinkEvent, bool>>
MultiSwitchThriftHandler::co_notifyLinkEvent(int64_t /*switchId*/) {
  co_return {};
}

folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::FdbEvent, bool>>
MultiSwitchThriftHandler::co_notifyFdbEvent(int64_t /*switchId*/) {
  co_return {};
}

folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::RxPacket, bool>>
MultiSwitchThriftHandler::co_notifyRxPacket(int64_t /*switchId*/) {
  co_return {};
}

folly::coro::Task<apache::thrift::ServerStream<fsdb::OperDelta>>
MultiSwitchThriftHandler::co_getStateUpdates(int64_t /*switchId*/) {
  co_return apache::thrift::ServerStream<fsdb::OperDelta>::createEmpty();
}

folly::coro::Task<apache::thrift::ServerStream<multiswitch::TxPacket>>
MultiSwitchThriftHandler::co_getTxPackets(int64_t /*switchId*/) {
  co_return apache::thrift::ServerStream<multiswitch::TxPacket>::createEmpty();
}
#endif
} // namespace facebook::fboss
