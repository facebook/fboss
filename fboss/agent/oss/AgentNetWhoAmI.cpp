// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentNetWhoAmI.h"

namespace facebook::fboss {

bool AgentNetWhoAmI::isSai() const {
  return false;
}

bool AgentNetWhoAmI::isBcmSaiPlatform() const {
  return false;
}

bool AgentNetWhoAmI::isCiscoSaiPlatform() const {
  return false;
}

bool AgentNetWhoAmI::isBcmPlatform() const {
  return false;
}

bool AgentNetWhoAmI::isCiscoPlatform() const {
  return false;
}

bool AgentNetWhoAmI::isBcmVoqPlatform() const {
  return false;
}

bool AgentNetWhoAmI::isFdsw() const {
  return false;
}

bool AgentNetWhoAmI::isSdsw() const {
  return false;
}

bool AgentNetWhoAmI::hasRoutingProtocol() const {
  return false;
}

bool AgentNetWhoAmI::hasBgpRoutingProtocol() const {
  return false;
}

bool AgentNetWhoAmI::isNotDrainable() const {
  return true;
}

} // namespace facebook::fboss
