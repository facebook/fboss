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

#include <folly/experimental/TestUtil.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <optional>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

#include <gmock/gmock.h>

namespace facebook::fboss {

class MockPlatformPort : public PlatformPort {
 public:
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
};

} // namespace facebook::fboss
