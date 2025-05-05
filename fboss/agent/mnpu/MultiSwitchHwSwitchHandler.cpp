// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/MultiSwitchHwSwitchHandler.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/MultiSwitchPacketStreamMap.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"

DEFINE_int32(oper_delta_ack_timeout, 600, "Oper delta ack timeout in seconds");

namespace facebook::fboss {

MultiSwitchHwSwitchHandler::MultiSwitchHwSwitchHandler(
    const SwitchID& switchId,
    const cfg::SwitchInfo& info,
    SwSwitch* sw)
    : HwSwitchHandler(switchId, info), sw_(sw) {}

std::unique_ptr<TxPacket> MultiSwitchHwSwitchHandler::allocatePacket(
    uint32_t size) const {
  return TxPacket::allocateTxPacket(size);
}

bool MultiSwitchHwSwitchHandler::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  return sendPacketOutViaThriftStream(std::move(pkt), portID, queue);
}

bool MultiSwitchHwSwitchHandler::sendPacketSwitchedSync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return sendPacketOutViaThriftStream(std::move(pkt));
}

bool MultiSwitchHwSwitchHandler::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return sendPacketOutViaThriftStream(std::move(pkt));
}

bool MultiSwitchHwSwitchHandler::transactionsSupported(
    std::optional<cfg::SdkVersion> sdkVersion) const {
  auto asicType = getSwitchInfo().asicType().value();
  if (asicType == cfg::AsicType::ASIC_TYPE_CHENAB) {
    return false;
  }
  if (asicType == cfg::AsicType::ASIC_TYPE_EBRO ||
      asicType == cfg::AsicType::ASIC_TYPE_YUBA) {
    return true;
  }
  if (sdkVersion.has_value() && sdkVersion.value().saiSdk().has_value()) {
    return true;
  }
  return false;
}

std::map<PortID, FabricEndpoint>
MultiSwitchHwSwitchHandler::getFabricConnectivity() const {
  // TODO - retire this after clients move to HwAgent thrift api
  return std::map<PortID, FabricEndpoint>();
}

std::vector<PortID> MultiSwitchHwSwitchHandler::getSwitchReachability(
    SwitchID /*switchId*/) const {
  return std::vector<PortID>();
}

FabricReachabilityStats MultiSwitchHwSwitchHandler::getFabricReachabilityStats()
    const {
  // TODO: implement this
  return {};
}

bool MultiSwitchHwSwitchHandler::needL2EntryForNeighbor(
    const cfg::SwitchConfig* config) const {
  if (cfg::AsicType::ASIC_TYPE_CHENAB == getSwitchInfo().asicType().value()) {
    return false;
  }
  // if config is not present, fall back to true
  // if sdk version is not set (for test configs), fall back to true
  // if sai, return true else false
  // falling back to true as assumption is split mode is for SAI alone
  return !config || !config->sdkVersion().has_value() ||
      config->sdkVersion()->saiSdk().has_value();
}

bool MultiSwitchHwSwitchHandler::sendPacketOutViaThriftStream(
    std::unique_ptr<TxPacket> pkt,
    std::optional<PortID> portID,
    std::optional<uint8_t> queue) {
  sw_->sendPacketOutViaThriftStream(
      std::move(pkt), getSwitchId(), portID, queue);
  return true;
}

bool MultiSwitchHwSwitchHandler::checkOperSyncStateLocked(
    HwSwitchOperDeltaSyncState state,
    const std::unique_lock<std::mutex>& /*lock*/) const {
  return operDeltaSyncState_ == state;
}

std::pair<fsdb::OperDelta, HwSwitchStateUpdateStatus>
MultiSwitchHwSwitchHandler::stateChanged(
    const fsdb::OperDelta& delta,
    bool transaction,
    const std::shared_ptr<SwitchState>& newState,
    const HwWriteBehavior& hwWriteBehavior) {
  multiswitch::StateOperDelta stateDelta;
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    SCOPE_EXIT {
      // clear any ack before waiting. initial sync might set ack
      // when there is no update thread waiting for ack
      prevOperDeltaResult_ = nullptr;
    };
    prevUpdateSwitchState_ = newState;
    if (checkOperSyncStateLocked(
            HwSwitchOperDeltaSyncState::DISCONNECTED, lk) ||
        checkOperSyncStateLocked(HwSwitchOperDeltaSyncState::CANCELLED, lk)) {
      // return incoming delta to indicate that none of the changes were applied
      return {
          delta, HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_CANCELLED};
    }
    // block state update till hwswitch resync is complete
    if (checkOperSyncStateLocked(
            HwSwitchOperDeltaSyncState::INITIAL_SYNC_SENT, lk) ||
        pauseStateUpdates_) {
      // Wait for initial sync to complete
      if (!waitForOperSyncAck(lk, FLAGS_oper_delta_ack_timeout)) {
        setOperSyncStateLocked(HwSwitchOperDeltaSyncState::CANCELLED, lk);
        // initial sync was cancelled
        return {
            delta, HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_CANCELLED};
      }
    }
    fillMultiswitchOperDelta(
        stateDelta,
        prevUpdateSwitchState_,
        delta,
        transaction,
        currOperDeltaSeqNum_,
        hwWriteBehavior);
    ++currOperDeltaSeqNum_;
    nextOperDelta_ = &stateDelta;
  }
  // state update ready. notify waiting thread
  stateUpdateCV_.notify_one();
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    SCOPE_EXIT {
      nextOperDelta_ = nullptr;
    };
    if (waitForOperSyncAck(lk, FLAGS_oper_delta_ack_timeout)) {
      SCOPE_EXIT {
        prevOperDeltaResult_ = nullptr;
      };
      // received ack. return result from HwSwitch
      return {
          *prevOperDeltaResult_->operDelta(),
          prevOperDeltaResult_->operDelta()->changes()->empty()
              ? HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_SUCCEEDED
              : HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_FAILED};
    } else {
      setOperSyncStateLocked(HwSwitchOperDeltaSyncState::CANCELLED, lk);
      // return incoming delta to indicate that none of the changes were applied
      return {
          delta, HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_CANCELLED};
    }
  }
}

multiswitch::StateOperDelta MultiSwitchHwSwitchHandler::getNextStateOperDelta(
    std::unique_ptr<multiswitch::StateOperDelta> prevOperResult,
    int64_t lastUpdateSeqNum) {
  bool serveOperSyncRequest{false};
  SCOPE_EXIT {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    // Reset the flag only if we are serving the request.
    if (serveOperSyncRequest) {
      operRequestInProgress_ = false;
      pauseStateUpdates_ = false;
    }
  };
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    // check if there is a pending oper delta request. This can happen if
    // client resends request due to thrift shutdown. Ignore the
    // new request and force full sync by setting state to cancelled.
    if (operRequestInProgress_) {
      XLOG(DBG2)
          << "Ignoring oper delta request as another request is in progress for hwswitch "
          << getSwitchId();
      // Reset the last acked seqnum to -1 so that next request results in full
      // sync
      lastAckedOperDeltaSeqNum_ = -1;
      // Indicate that new state updates are blocked till old request times out
      pauseStateUpdates_ = true;
      multiswitch::StateOperDelta cancelledResponse;
      cancelledResponse.seqNum() = 0;
      return cancelledResponse;
    }
    serveOperSyncRequest = true;
    operRequestInProgress_ = true;
    if (!checkOperSyncStateLocked(HwSwitchOperDeltaSyncState::CANCELLED, lk) &&
        (lastUpdateSeqNum == lastAckedOperDeltaSeqNum_)) {
      // HwSwitch has resent an ack for the previous delta. This indicates
      // that hwswitch timedout waiting for a new oper delta to be available.
      // Wait for a new delta to be available.
    } else if (
        !lastUpdateSeqNum || (lastUpdateSeqNum != currOperDeltaSeqNum_) ||
        checkOperSyncStateLocked(HwSwitchOperDeltaSyncState::CANCELLED, lk)) {
      // Mark initial sync needed if seq number from client is 0 or
      // mismatches with the current expected sequence number
      XLOG(DBG2) << "Need resync for hwswitch:" << getSwitchId()
                 << " last seen seqnum=" << lastUpdateSeqNum
                 << " curr seqnum=" << currOperDeltaSeqNum_;
      sw_->getHwSwitchHandler()->connected(getSwitchId());
      // If HwSwitchHandler has a valid state, send full sync delta
      if (prevUpdateSwitchState_) {
        setOperSyncStateLocked(
            HwSwitchOperDeltaSyncState::INITIAL_SYNC_SENT, lk);
        multiswitch::StateOperDelta fullOperResponse;
        fullOperResponse.seqNum() = ++currOperDeltaSeqNum_;
        fullOperResponse.operDelta() =
            getFullSyncOperDelta(prevUpdateSwitchState_);
        fullOperResponse.isFullState() = true;
        return fullOperResponse;
      } else {
        // Swswitch received an operdelta request before it had a chance to set
        // the first switch state. Mark hwswitchsyncer state as connected so
        // that next state update will include this HwSwitch. The first
        // operdelta will be a full sync delta. ie
        // StateDelta(std::make_shared<SwitchState>(), initialState). So it is
        // fine to send the first operdelta as a full sync delta.
        setOperSyncStateLocked(HwSwitchOperDeltaSyncState::CONNECTED, lk);
      }
    } else {
      if (checkOperSyncStateLocked(
              HwSwitchOperDeltaSyncState::INITIAL_SYNC_SENT, lk)) {
        setOperSyncStateLocked(HwSwitchOperDeltaSyncState::CONNECTED, lk);
      }
      prevOperDeltaResult_ = prevOperResult.get();
    }
    lastAckedOperDeltaSeqNum_ = lastUpdateSeqNum;
  }
  stateUpdateCV_.notify_one();
  // wait for new delta to be available or for cancellation.
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    SCOPE_EXIT {
      prevOperDeltaResult_ = nullptr;
    };
    // Wait for either delta to be available or for cancellation.
    // Clients does a timed wait for response from swswitch to avoid waiting
    // forever if the server crashes. Server also needs to do a timed wait
    // so that the worker threads corresponding to timed out clients do not
    // block on server forever
    if (nextOperDelta_ ||
        waitForOperDeltaReady(lk, FLAGS_oper_sync_req_timeout)) {
      SCOPE_EXIT {
        nextOperDelta_ = nullptr;
      };
      return *nextOperDelta_;
    } else {
      // return empty delta to HwSwitch incase of cancellation or timeout.
      // cancellation occurs when the client disconnects or when server
      // undergoes a graceful shutdown.
      multiswitch::StateOperDelta cancelledResponse;
      cancelledResponse.seqNum() = currOperDeltaSeqNum_;
      return cancelledResponse;
    }
  }
}

void MultiSwitchHwSwitchHandler::notifyHwSwitchDisconnected() {
  // cancel any pending operations.
  cancelOperDeltaSync();
}

void MultiSwitchHwSwitchHandler::cancelOperDeltaSync() {
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    setOperSyncStateLocked(HwSwitchOperDeltaSyncState::CANCELLED, lk);
    nextOperDelta_ = nullptr;
  }
  stateUpdateCV_.notify_all();
}

MultiSwitchHwSwitchHandler::~MultiSwitchHwSwitchHandler() {
  cancelOperDeltaSync();
}

HwSwitchOperDeltaSyncState
MultiSwitchHwSwitchHandler::getHwSwitchOperDeltaSyncState() {
  std::unique_lock<std::mutex> lk(stateUpdateMutex_);
  return operDeltaSyncState_;
}

SwitchRunState MultiSwitchHwSwitchHandler::getHwSwitchRunState() {
  HwSwitchOperDeltaSyncState syncState;
  {
    std::unique_lock<std::mutex> lk(stateUpdateMutex_);
    syncState = getOperSyncState();
  }

  switch (syncState) {
    case HwSwitchOperDeltaSyncState::DISCONNECTED:
      return SwitchRunState::UNINITIALIZED;
    case HwSwitchOperDeltaSyncState::INITIAL_SYNC_SENT:
      return SwitchRunState::INITIALIZED;
    case HwSwitchOperDeltaSyncState::CONNECTED:
      return SwitchRunState::CONFIGURED;
    case HwSwitchOperDeltaSyncState::CANCELLED:
      return SwitchRunState::EXITING;
  }
  throw FbossError("Unknown hw switch run state");
}

bool MultiSwitchHwSwitchHandler::waitForOperSyncAck(
    std::unique_lock<std::mutex>& lk,
    uint64_t timeoutInSec) {
  if (!stateUpdateCV_.wait_for(
          lk, std::chrono::seconds(timeoutInSec), [this, &lk] {
            return (
                prevOperDeltaResult_ ||
                checkOperSyncStateLocked(
                    HwSwitchOperDeltaSyncState::CANCELLED, lk));
          })) {
    XLOG(DBG2) << "Timed out waiting oper delta ack from switch "
               << getSwitchId();
    operDeltaAckTimeout();
    sw_->getHwSwitchHandler()->disconnected(getSwitchId());
    return false;
  }
  return checkOperSyncStateLocked(HwSwitchOperDeltaSyncState::CANCELLED, lk)
      ? false
      : true;
}

bool MultiSwitchHwSwitchHandler::waitForOperDeltaReady(
    std::unique_lock<std::mutex>& lk,
    uint64_t timeoutInSec) {
  if (!stateUpdateCV_.wait_for(
          lk, std::chrono::seconds(timeoutInSec), [this, &lk] {
            return (
                nextOperDelta_ ||
                checkOperSyncStateLocked(
                    HwSwitchOperDeltaSyncState::CANCELLED, lk));
          })) {
    XLOG(DBG3) << "Timed out waiting for next state oper delta for switch "
               << getSwitchId();
    return false;
  }
  return checkOperSyncStateLocked(HwSwitchOperDeltaSyncState::CANCELLED, lk)
      ? false
      : true;
}

void MultiSwitchHwSwitchHandler::fillMultiswitchOperDelta(
    multiswitch::StateOperDelta& stateDelta,
    const std::shared_ptr<SwitchState>& state,
    const fsdb::OperDelta& delta,
    bool transaction,
    int64_t lastSeqNum,
    const HwWriteBehavior& hwWriteBehavior) {
  // Send full delta if this is first switchstate update.
  // Sequence number 0 indicates first update
  if (lastSeqNum == 0) {
    stateDelta.isFullState() = true;
    stateDelta.operDelta() = getFullSyncOperDelta(state);
    CHECK(!transaction);
  } else {
    stateDelta.isFullState() = false;
    stateDelta.operDelta() = delta;
  }
  stateDelta.transaction() = transaction;
  stateDelta.seqNum() = lastSeqNum + 1;
  stateDelta.hwWriteBehavior() = hwWriteBehavior;
}

void MultiSwitchHwSwitchHandler::operDeltaAckTimeout() {
  auto switchIndex =
      sw_->getSwitchInfoTable().getSwitchIndexFromSwitchId(getSwitchId());
  sw_->stats()->hwAgentUpdateTimeout(switchIndex);
}

state::SwitchState MultiSwitchHwSwitchHandler::reconstructSwitchState() {
  throw FbossError(
      "reconstructSwitchState Not implemented in MultiSwitchHwSwitchHandler");
}

} // namespace facebook::fboss
