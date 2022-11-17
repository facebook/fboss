// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/IPAddress.h>
#include <sys/types.h>
#include <cstdint>
#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

namespace facebook::fboss {

struct ConcurrentIndices;
class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiTunnel = SaiObject<SaiTunnelTraits>;
using SaiP2MPTunnelTerm = SaiObject<SaiP2MPTunnelTermTraits>;
using SaiP2PTunnelTerm = SaiObject<SaiP2PTunnelTermTraits>;

struct SaiTunnelHandle {
  std::shared_ptr<SaiTunnel> tunnel;
  using SaiTunnelTerm = std::variant<
      std::shared_ptr<SaiP2MPTunnelTerm>,
      std::shared_ptr<SaiP2PTunnelTerm>>;
  SaiTunnelTerm tunnelTerm;
  TunnelTermSaiId adapterKey() {
    return std::visit(
        [](auto& handle) { return handle->adapterKey(); }, tunnelTerm);
  }
  std::shared_ptr<SaiP2MPTunnelTerm> getP2MPTunnelTermHandle() const {
    auto* tunnelTermHandle =
        std::get_if<std::shared_ptr<SaiP2MPTunnelTerm>>(&tunnelTerm);
    if (!tunnelTermHandle) {
      return nullptr;
    }
    return *tunnelTermHandle;
  }
};

class SaiTunnelManager {
  using Handles =
      folly::F14FastMap<std::string, std::unique_ptr<SaiTunnelHandle>>;

 public:
  SaiTunnelManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* /*platform*/);
  ~SaiTunnelManager();
  TunnelSaiId addTunnel(const std::shared_ptr<IpTunnel>& swTunnel);
  void removeTunnel(const std::shared_ptr<IpTunnel>& swTunnel);
  void changeTunnel(
      const std::shared_ptr<IpTunnel>& oldTunnel,
      const std::shared_ptr<IpTunnel>& newTunnel);

  const SaiTunnelHandle* getTunnelHandle(std::string swId) const {
    return getTunnelHandleImpl(swId);
  }
  SaiTunnelHandle* getTunnelHandle(std::string swId) {
    return getTunnelHandleImpl(swId);
  }

 private:
  SaiTunnelHandle* getTunnelHandleImpl(std::string swId) const;
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  Handles handles_;
};

} // namespace facebook::fboss
