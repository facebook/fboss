// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {

class AgentNetWhoAmI {
 public:
  AgentNetWhoAmI() {}
  virtual ~AgentNetWhoAmI() = default;
  virtual bool isSai() const;
  virtual bool isBcmSaiPlatform() const;
  virtual bool isTajoSaiPlatform() const;
  virtual bool isBcmPlatform() const;
  virtual bool isTajoPlatform() const;
  virtual bool isChenabPlatform() const;
  virtual bool isBcmVoqPlatform() const;
  virtual bool isTajoMorgan800ccPlatform() const;
  virtual bool isFdsw() const;
  virtual bool isSdsw() const;
  virtual bool isNotDrainable() const;
  virtual bool hasRoutingProtocol() const;
  virtual bool hasBgpRoutingProtocol() const;
};

} // namespace facebook::fboss
