// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <memory>

#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

void MultiSwitchThriftHandler::ensureConfigured(
    folly::StringPiece function) const {
  if (sw_->isFullyConfigured()) {
    return;
  }

  if (!function.empty()) {
    XLOG(DBG1) << "failing thrift prior to switch configuration: " << function;
  }
  throw FbossError(
      "switch is still initializing or is exiting and is not "
      "fully configured yet");
}

#if FOLLY_HAS_COROUTINES
folly::coro::Task<apache::thrift::SinkConsumer<fsdb::OperDelta, bool>>
MultiSwitchThriftHandler::co_notifyStateUpdateResult(int64_t /*switchId*/) {
  co_return {};
}

folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::LinkEvent, bool>>
MultiSwitchThriftHandler::co_notifyLinkEvent(int64_t switchId) {
  ensureConfigured(__func__);
  co_return apache::thrift::SinkConsumer<multiswitch::LinkEvent, bool>{
      [this,
       switchId](folly::coro::AsyncGenerator<multiswitch::LinkEvent&&> gen)
          -> folly::coro::Task<bool> {
        while (auto item = co_await gen.next()) {
          XLOG(DBG3) << "Got link event from switch " << switchId
                     << " for port " << *item->port() << " up :" << *item->up();
          PortID portId = PortID(*item->port());
          sw_->linkStateChanged(portId, *item->up());
        }
        co_return true;
      },
      10 /* buffer size */
  };
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
