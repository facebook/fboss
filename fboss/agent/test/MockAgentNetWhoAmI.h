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
  MOCK_CONST_METHOD0(isCiscoMorgan800ccPlatform, bool());
  MOCK_CONST_METHOD0(isBcmVoqPlatform, bool());
  MOCK_CONST_METHOD0(isFdsw, bool());
  MOCK_CONST_METHOD0(isSdsw, bool());
  MOCK_CONST_METHOD0(isNotDrainable, bool());
  MOCK_CONST_METHOD0(hasRoutingProtocol, bool());
  MOCK_CONST_METHOD0(hasBgpRoutingProtocol, bool());
};

} // namespace facebook::fboss
