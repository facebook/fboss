// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>

#include "fboss/agent/AgentNetWhoAmI.h"

namespace facebook::fboss {

class MockAgentNetWhoAmI : public AgentNetWhoAmI {
 public:
  MOCK_CONST_METHOD0(isSai, bool());
  MOCK_CONST_METHOD0(isBcmSaiPlatform, bool());
  MOCK_CONST_METHOD0(isCiscoSaiPlatform, bool());
  MOCK_CONST_METHOD0(isBcmPlatform, bool());
  MOCK_CONST_METHOD0(isCiscoPlatform, bool());
  MOCK_CONST_METHOD0(isBcmVoqPlatform, bool());
  MOCK_CONST_METHOD0(isFdsw, bool());
  MOCK_CONST_METHOD0(isUnDrainable, bool());
  MOCK_CONST_METHOD0(hasRoutingProtocol, bool());
  MOCK_CONST_METHOD0(hasBgpRoutingProtocol, bool());
};

} // namespace facebook::fboss
