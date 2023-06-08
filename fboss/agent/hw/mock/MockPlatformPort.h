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

#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

#include <gmock/gmock.h>

namespace facebook::fboss {

class MockPlatform;

class MockPlatformPort : public PlatformPort {
 public:
  MockPlatformPort(PortID id, MockPlatform* platform)
      : PlatformPort(id, platform) {}
  MOCK_CONST_METHOD0(getPortID, PortID());
  MOCK_METHOD1(preDisable, void(bool temporary));
  MOCK_METHOD1(postDisable, void(bool temporary));
  MOCK_METHOD0(preEnable, void());
  MOCK_METHOD0(postEnable, void());
  MOCK_METHOD0(isMediaPresent, bool());
  MOCK_METHOD2(linkStatusChanged, void(bool up, bool adminUp));
  MOCK_CONST_METHOD0(supportsTransceiver, bool());
  MOCK_CONST_METHOD0(getTransceiverID, std::optional<TransceiverID>());
  MOCK_METHOD6(
      statusIndication,
      void(
          bool enabled,
          bool link,
          bool ingress,
          bool egress,
          bool discards,
          bool errors));
  MOCK_METHOD0(prepareForGracefulExit, void());
  MOCK_CONST_METHOD0(shouldDisableFEC, bool());
  MOCK_METHOD1(externalState, void(PortLedExternalState));
  MOCK_CONST_METHOD0(
      getFutureTransceiverInfo,
      folly::Future<TransceiverInfo>());
  std::shared_ptr<TransceiverSpec> getTransceiverSpec() const override {
    return nullptr;
  }
  std::optional<int> getAttachedCoreId() const override {
    return 0;
  }
  std::optional<int> getCorePortIndex() const override {
    return getPortID();
  }
};

} // namespace facebook::fboss
