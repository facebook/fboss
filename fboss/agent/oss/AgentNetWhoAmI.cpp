// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentNetWhoAmI.h"

namespace facebook::fboss {

namespace detail {
struct AgentNetWhoAmIImpl {};
} // namespace detail

AgentNetWhoAmI::AgentNetWhoAmI() {
  impl_ = std::make_unique<detail::AgentNetWhoAmIImpl>();
}

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

bool AgentNetWhoAmI::hasRoutingProtocol() const {
  return false;
}

bool AgentNetWhoAmI::hasBgpRoutingProtocol() const {
  return false;
}

bool AgentNetWhoAmI::isUnDrainable() const {
  return true;
}

} // namespace facebook::fboss
