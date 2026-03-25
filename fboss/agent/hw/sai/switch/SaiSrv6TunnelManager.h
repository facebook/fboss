// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/Srv6Tunnel.h"

#include "folly/container/F14Map.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
using SaiSrv6Tunnel = SaiObject<SaiSrv6TunnelTraits>;
#endif

struct SaiSrv6TunnelHandle {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::shared_ptr<SaiSrv6Tunnel> tunnel;
#endif
};

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

 private:
  SaiSrv6TunnelHandle* getSrv6TunnelHandleImpl(const std::string& swId) const;
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  Handles handles_;
};

} // namespace facebook::fboss
