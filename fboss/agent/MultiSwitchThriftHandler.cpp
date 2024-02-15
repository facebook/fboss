// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <memory>

#include "fboss/agent/MultiSwitchPacketStreamMap.h"
#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/SwRxPacket.h"
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

L2Entry MultiSwitchThriftHandler::getL2Entry(L2EntryThrift thriftEntry) {
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
          std::optional<phy::LinkFaultStatus> faultStatus;
          if (item->iPhyLinkFaultStatus()) {
            faultStatus = *item->iPhyLinkFaultStatus();
          }
          sw_->linkStateChanged(portId, *item->up(), faultStatus);
        }
        co_return true;
      },
      10 /* buffer size */
  }
      .setChunkTimeout(std::chrono::milliseconds(0));
}

folly::coro::Task<
    apache::thrift::SinkConsumer<multiswitch::LinkActiveEvent, bool>>
MultiSwitchThriftHandler::co_notifyLinkActiveEvent(int64_t switchId) {
  ensureConfigured(__func__);
  co_return apache::thrift::SinkConsumer<multiswitch::LinkActiveEvent, bool>{
      [this, switchId](
          folly::coro::AsyncGenerator<multiswitch::LinkActiveEvent&&> gen)
          -> folly::coro::Task<bool> {
        std::map<PortID, bool> port2IsActive;
        while (auto item = co_await gen.next()) {
          XLOG(DBG3) << "Got link active event from switch " << switchId;
          for (const auto& [portID, isActive] : *item->port2IsActive()) {
            port2IsActive[PortID(portID)] = isActive;
          }
          sw_->linkActiveStateChanged(port2IsActive);
        }
        co_return true;
      },
      10 /* buffer size */
  }
      .setChunkTimeout(std::chrono::milliseconds(0));
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
  }
      .setChunkTimeout(std::chrono::milliseconds(0));
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
          auto pkt = make_unique<SwRxPacket>(std::move(*item->data()));
          pkt->setSrcPort(PortID(*item->port()));
          if (item->vlan()) {
            pkt->setSrcVlan(VlanID(*item->vlan()));
          } else {
            // clear default vlan id(0)
            // TODO - retire this once the default value for vlan id is removed
            pkt->setSrcVlan(std::nullopt);
          }
          if (item->aggPort()) {
            pkt->setSrcAggregatePort(AggregatePortID(*item->aggPort()));
          }
          sw_->packetReceived(std::move(pkt));
        }
        co_return true;
      },
      1000 /* buffer size */
  }
      .setChunkTimeout(std::chrono::milliseconds(0));
}

folly::coro::Task<apache::thrift::ServerStream<multiswitch::TxPacket>>
MultiSwitchThriftHandler::co_getTxPackets(int64_t switchId) {
  auto streamAndPublisher =
      apache::thrift::ServerStream<multiswitch::TxPacket>::createPublisher(
          [this, switchId] {
            sw_->getPacketStreamMap()->removePacketStream(SwitchID(switchId));
            XLOG(DBG2) << "Removed stream for switch " << switchId;
            sw_->getHwSwitchHandler()->notifyHwSwitchDisconnected(
                switchId, false);
          });
  auto streamPublisher = std::make_unique<
      apache::thrift::ServerStreamPublisher<multiswitch::TxPacket>>(
      std::move(streamAndPublisher.second));
  sw_->getPacketStreamMap()->addPacketStream(
      SwitchID(switchId), std::move(streamPublisher));
  co_return std::move(streamAndPublisher.first);
}

folly::coro::Task<
    apache::thrift::SinkConsumer<multiswitch::HwSwitchStats, bool>>
MultiSwitchThriftHandler::co_syncHwStats(int16_t switchIndex) {
  ensureConfigured(__func__);
  co_return apache::thrift::SinkConsumer<multiswitch::HwSwitchStats, bool>{
      [this, switchIndex](
          folly::coro::AsyncGenerator<multiswitch::HwSwitchStats&&> gen)
          -> folly::coro::Task<bool> {
        while (auto item = co_await gen.next()) {
          XLOG(DBG3) << "Got stats event from switchIndex " << switchIndex;
          sw_->updateHwSwitchStats(switchIndex, std::move(*item));
        }
        co_return true;
      },
      100 /* buffer size */
  };
}

#endif

void MultiSwitchThriftHandler::getNextStateOperDelta(
    multiswitch::StateOperDelta& operDelta,
    int64_t switchId,
    std::unique_ptr<multiswitch::StateOperDelta> prevOperResult,
    int64_t lastUpdateSeqNum) {
  operDelta = sw_->getHwSwitchHandler()->getNextStateOperDelta(
      switchId, std::move(prevOperResult), lastUpdateSeqNum);
}

void MultiSwitchThriftHandler::gracefulExit(int64_t switchId) {
  sw_->getHwSwitchHandler()->notifyHwSwitchGracefulExit(switchId);
}

} // namespace facebook::fboss
