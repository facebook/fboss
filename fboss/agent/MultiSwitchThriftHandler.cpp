// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <memory>

#include "fboss/agent/MultiSwitchPacketStreamMap.h"
#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
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

L2Entry MultiSwitchThriftHandler::getL2Entry(L2EntryThrift thriftEntry) const {
  PortDescriptor port = thriftEntry.trunk()
      ? PortDescriptor(AggregatePortID(*thriftEntry.port()))
      : PortDescriptor(PortID(*thriftEntry.port()));
  std::optional<cfg::AclLookupClass> classID;
  if (thriftEntry.classID()) {
    classID = cfg::AclLookupClass(*thriftEntry.classID());
  }
  L2Entry::L2EntryType type =
      *thriftEntry.l2EntryType() == L2EntryType::L2_ENTRY_TYPE_PENDING
      ? L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING
      : L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED;
  return L2Entry(
      folly::MacAddress(*thriftEntry.mac()),
      VlanID(*thriftEntry.vlanID()),
      port,
      type,
      classID);
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
MultiSwitchThriftHandler::co_notifyFdbEvent(int64_t switchId) {
  ensureConfigured(__func__);
  co_return apache::thrift::SinkConsumer<multiswitch::FdbEvent, bool>{
      [this, switchId](folly::coro::AsyncGenerator<multiswitch::FdbEvent&&> gen)
          -> folly::coro::Task<bool> {
        while (auto item = co_await gen.next()) {
          XLOG(DBG3) << "Got fdb event from switch " << switchId << " for port "
                     << *item->entry()->port()
                     << " mac :" << *item->entry()->mac();
          auto l2Entry = getL2Entry(*item->entry());
          sw_->l2LearningUpdateReceived(l2Entry, *item->updateType());
        }
        co_return true;
      },
      10 /* buffer size */
  };
}

folly::coro::Task<apache::thrift::SinkConsumer<multiswitch::RxPacket, bool>>
MultiSwitchThriftHandler::co_notifyRxPacket(int64_t switchId) {
  ensureConfigured(__func__);
  co_return apache::thrift::SinkConsumer<multiswitch::RxPacket, bool>{
      [this, switchId](folly::coro::AsyncGenerator<multiswitch::RxPacket&&> gen)
          -> folly::coro::Task<bool> {
        while (auto item = co_await gen.next()) {
          XLOG(DBG4) << "Got rx packet from switch " << switchId << " for port "
                     << *item->port();
          auto pkt = make_unique<MockRxPacket>(std::move(*item->data()));
          pkt->setSrcPort(PortID(*item->port()));
          if (item->vlan()) {
            pkt->setSrcVlan(VlanID(*item->vlan()));
          }
          sw_->packetReceived(std::move(pkt));
        }
        co_return true;
      },
      1000 /* buffer size */
  };
}

folly::coro::Task<apache::thrift::ServerStream<fsdb::OperDelta>>
MultiSwitchThriftHandler::co_getStateUpdates(int64_t /*switchId*/) {
  co_return apache::thrift::ServerStream<fsdb::OperDelta>::createEmpty();
}

folly::coro::Task<apache::thrift::ServerStream<multiswitch::TxPacket>>
MultiSwitchThriftHandler::co_getTxPackets(int64_t switchId) {
  auto streamAndPublisher =
      apache::thrift::ServerStream<multiswitch::TxPacket>::createPublisher(
          [this, switchId] {
            sw_->getPacketStreamMap()->removePacketStream(SwitchID(switchId));
            XLOG(DBG2) << "Removed stream for switch " << switchId;
          });
  auto streamPublisher = std::make_unique<
      apache::thrift::ServerStreamPublisher<multiswitch::TxPacket>>(
      std::move(streamAndPublisher.second));
  sw_->getPacketStreamMap()->addPacketStream(
      SwitchID(switchId), std::move(streamPublisher));
  co_return std::move(streamAndPublisher.first);
}
#endif
} // namespace facebook::fboss
