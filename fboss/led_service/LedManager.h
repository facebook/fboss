/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/Format.h>
#include <folly/Synchronized.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <stdexcept>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/led_service/FsdbSwitchStateSubscriber.h"
#include "fboss/led_service/LedConfig.h"
#include "fboss/led_service/LedUtils.h"
#include "fboss/lib/led/LedIO.h"
#include "fboss/lib/led/gen-cpp2/led_mapping_types.h"

namespace facebook::fboss {

/*
 * LedManager class definiton:
 *
 * The LedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class LedManager {
  using PortDisplayInfo = struct PortDisplayInfo {
    std::string portName;
    cfg::PortProfileID portProfileId;
    bool operationStateUp{false};
    bool cablingError{false};
    bool forcedOn{false};
    bool forcedOff{false};
    led::LedState currentLedState{utility::constructLedState(
        led::LedColor::UNKNOWN,
        led::Blink::UNKNOWN)};
    std::optional<bool> activeState{std::nullopt};
    bool drained{false};
  };

 public:
  using LedSwitchStateUpdate = struct LedSwitchStateUpdate {
    short swPortId;
    std::string portName;
    std::string portProfile;
    bool operState;
    std::optional<PortLedExternalState> ledExternalState;
    std::optional<bool> activeState{std::nullopt};
    bool drained;
  };

  LedManager();
  virtual ~LedManager();

  // Initialize the Led Manager, get system container
  virtual void initLedManager() {}

  // On getting the update from FSDB, update portDisplayMap_
  void updateLedStatus(std::map<short, LedSwitchStateUpdate> newSwitchState);

  folly::EventBase* getEventBase() {
    return eventBase_.get();
  }

  fsdb::FsdbPubSubManager* pubSubMgr() const {
    return fsdbPubSubMgr_.get();
  }

  void setExternalLedState(int32_t portNum, PortLedExternalState ledState);

  led::PortLedState getPortLedState(const std::string& swPortName) const;

  bool isLedControlledThroughService() const;

  led::LedColor getCurrentLedColor(int32_t portNum) const;

  // Derived platforms can set the forcedforcedOnColor
  virtual led::LedColor forcedOnColor() const {
    return led::LedColor::BLUE;
  }

  const PlatformMapping* getPlatformMapping() const {
    return platformMapping_.get();
  }

  virtual bool blinkingSupported() const {
    return false;
  }

  // Forbidden copy constructor and assignment operator
  LedManager(LedManager const&) = delete;
  LedManager& operator=(LedManager const&) = delete;

 protected:
  // Platform mapping to get SW Port -> Lane mapping
  std::unique_ptr<PlatformMapping> platformMapping_;

  // Port Name to PortDisplayInfo map, no lock needed
  std::map<uint32_t, PortDisplayInfo> portDisplayMap_;

  std::unique_ptr<FsdbSwitchStateSubscriber> fsdbSwitchStateSubscriber_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;

  std::unique_ptr<LedConfig> ledConfig_;

  virtual led::LedState calculateLedState(
      uint32_t /* portId */,
      cfg::PortProfileID /* portProfiled */) const = 0;

  virtual void setLedState(
      uint32_t /* portId */,
      cfg::PortProfileID /* portProfile */,
      led::LedState /* ledState */) = 0;

 private:
  std::unique_ptr<std::thread> ledManagerThread_{nullptr};
  std::unique_ptr<folly::EventBase> eventBase_;
};

} // namespace facebook::fboss
