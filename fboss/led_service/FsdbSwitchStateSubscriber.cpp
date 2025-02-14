// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/FsdbSwitchStateSubscriber.h"
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/led_service/LedManager.h"

namespace facebook::fboss {

/*
 * subscribeToSwitchState
 *
 * This function subscribes the state callback to FSDB Port Info updates. The
 * callback will update the Led Manager synchronized port info map
 */
void FsdbSwitchStateSubscriber::subscribeToSwitchState(LedManager* ledManager) {
  subscribeToState(getSwitchStatePath(), ledManager);
}

/*
 * removeSwitchStateSubscription
 *
 * This function removes the switch state subscription from FSDB
 */
void FsdbSwitchStateSubscriber::removeSwitchStateSubscription() {
  removeStateSubscribe(getSwitchStatePath());
}

/*
 * subscribeToState
 *
 * A helper function to subscribe to the state callback to FSDB for updates on
 * a given path.
 */
void FsdbSwitchStateSubscriber::subscribeToState(
    std::vector<std::string> path,
    LedManager* ledManager) {
  // Subscribe to FSDB only if the LED config is enabled
  if (!ledManager || !ledManager->isLedControlledThroughService()) {
    XLOG(ERR) << "LED Service is not subscribed to FSDB";
    return;
  }

  auto stateCb = [](fsdb::SubscriptionState /*old*/,
                    fsdb::SubscriptionState /*new*/,
                    std::optional<bool> /*initialSyncHasData*/) {};
  auto dataCb = [=](fsdb::OperState&& state) {
    if (auto contents = state.contents()) {
      // Deserialize the FSDB update to switch state struct. This will be
      // used by LED manager thread later
      auto newSwitchStateData = apache::thrift::BinarySerializer::deserialize<
          fboss::state::SwitchState>(*contents);
      auto& swSettings = newSwitchStateData.switchSettingsMap().value();
      auto swPortMaps = newSwitchStateData.portMaps().value();

      std::map<short, LedManager::LedSwitchStateUpdate> ledSwitchStateUpdate;

      for (auto& [switchStr, oneSwPortMap] : swPortMaps) {
        bool switchDrained = false;
        bool portActive = true;
        if (swSettings.find(switchStr) != swSettings.end()) {
          switchDrained =
              swSettings[switchStr].actualSwitchDrainState().value() ==
              cfg::SwitchDrainState::DRAINED;
        }
        for (auto& [onePortId, onePortInfo] : oneSwPortMap) {
          ledSwitchStateUpdate[onePortId].swPortId = onePortId;
          ledSwitchStateUpdate[onePortId].portName =
              onePortInfo.portName().value();
          ledSwitchStateUpdate[onePortId].portProfile =
              onePortInfo.portProfileID().value();
          ledSwitchStateUpdate[onePortId].operState =
              onePortInfo.portOperState().value();
          if (onePortInfo.portLedExternalState().has_value()) {
            ledSwitchStateUpdate[onePortId].ledExternalState =
                onePortInfo.portLedExternalState().value();
          }
          if (auto activeState = onePortInfo.portActiveState()) {
            ledSwitchStateUpdate[onePortId].activeState = *activeState;
            portActive = *activeState;
          }

          // We consider the port drained if either the local port is
          // drained or the local switch is drained or the peer port is drained.
          // Peer port is considered drained when the local port is inactive but
          // has expected neighbor. We blink the LED when either end of the link
          // is drained and hence is safe to operate on.
          bool localPortDrained =
              onePortInfo.get_drainState() == cfg::PortDrainState::DRAINED ||
              switchDrained;
          bool peerPortDrained = !portActive &&
              std::find(
                  onePortInfo.activeErrors()->begin(),
                  onePortInfo.activeErrors()->end(),
                  PortError::MISSING_EXPECTED_NEIGHBOR) ==
                  onePortInfo.activeErrors()->end();
          ledSwitchStateUpdate[onePortId].drained =
              localPortDrained || peerPortDrained;
        }
      }

      if (ledManager) {
        folly::via(ledManager->getEventBase()).thenValue([=](auto&&) {
          ledManager->updateLedStatus(ledSwitchStateUpdate);
        });

      } else {
        XLOG(ERR) << "Subscribed data came for invalid LED Manager";
      }
    }
  };
  pubSubMgr()->addStatePathSubscription(path, stateCb, dataCb);
  XLOG(INFO) << "LED Service Subscribed to FSDB switch state path";
}

/*
 * removeStateSubscribe
 *
 * This function removes the subscription from FSDB for the given FSDB state
 * path
 */
void FsdbSwitchStateSubscriber::removeStateSubscribe(
    std::vector<std::string> path) {
  pubSubMgr()->removeStatePathSubscription(path);
  XLOG(INFO) << "LED Service Removed from FSDB subscription";
}

} // namespace facebook::fboss
