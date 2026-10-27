// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/VxlanTunnel.h"

#include "folly/CppAttributes.h"
#include "folly/container/F14Map.h"

#include <memory>
#include <string>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiVxlanTunnel = SaiObject<SaiVxlanTunnelTraits>;

struct SaiVxlanTunnelHandle {
  std::shared_ptr<SaiVxlanTunnel> tunnel;
};

class SaiVxlanTunnelManager {
  using Handles =
      folly::F14FastMap<std::string, std::unique_ptr<SaiVxlanTunnelHandle>>;

 public:
  SaiVxlanTunnelManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);
  ~SaiVxlanTunnelManager();

  void addVxlanTunnel(const std::shared_ptr<VxlanTunnel>& vxlanTunnel);
  void removeVxlanTunnel(const std::shared_ptr<VxlanTunnel>& vxlanTunnel);
  void changeVxlanTunnel(
      const std::shared_ptr<VxlanTunnel>& oldTunnel,
      const std::shared_ptr<VxlanTunnel>& newTunnel);

  const SaiVxlanTunnelHandle* getVxlanTunnelHandle(
      const std::string& swId) const {
    return getVxlanTunnelHandleImpl(swId);
  }

 private:
  SaiVxlanTunnelHandle* getVxlanTunnelHandleImpl(const std::string& swId) const;
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  Handles handles_;
};

} // namespace facebook::fboss
