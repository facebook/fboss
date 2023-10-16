// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>

namespace facebook::fboss {

namespace detail {
struct AgentNetWhoAmIImpl;
}

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
  bool hasRoutingProtocol() const;
  bool hasBgpRoutingProtocol() const;

 private:
  std::unique_ptr<detail::AgentNetWhoAmIImpl> impl_;
};

} // namespace facebook::fboss
