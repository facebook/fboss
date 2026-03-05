// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/state/Srv6Tunnel.h"

#include "folly/container/F14Map.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

struct SaiSrv6TunnelHandle {};

class SaiSrv6TunnelManager {
  using Handles =
      folly::F14FastMap<std::string, std::unique_ptr<SaiSrv6TunnelHandle>>;

 public:
  SaiSrv6TunnelManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);
  ~SaiSrv6TunnelManager();

  void addSrv6Tunnel(const std::shared_ptr<Srv6Tunnel>& srv6Tunnel);
  void removeSrv6Tunnel(const std::shared_ptr<Srv6Tunnel>& srv6Tunnel);
  void changeSrv6Tunnel(
      const std::shared_ptr<Srv6Tunnel>& oldTunnel,
      const std::shared_ptr<Srv6Tunnel>& newTunnel);

  const SaiSrv6TunnelHandle* getSrv6TunnelHandle(
      const std::string& swId) const {
    return getSrv6TunnelHandleImpl(swId);
  }
  SaiSrv6TunnelHandle* getSrv6TunnelHandle(const std::string& swId) {
    return getSrv6TunnelHandleImpl(swId);
  }

 private:
  SaiSrv6TunnelHandle* getSrv6TunnelHandleImpl(const std::string& swId) const;
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  Handles handles_;
};

} // namespace facebook::fboss
