// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>

namespace facebook::fboss {

class AgentNetWhoAmI {
 public:
  AgentNetWhoAmI();
  bool isSai() const;
  bool isBcmSaiPlatform() const;
  bool isCiscoSaiPlatform() const;
  bool isBcmPlatform() const;
  bool isCiscoPlatform() const;
  bool isBcmVoqPlatform() const;
  bool isFdsw() const;
  bool isUnDrainable() const;
  bool hasRoutingProtocol() const;
  bool hasBgpRoutingProtocol() const;
};

} // namespace facebook::fboss
