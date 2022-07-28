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
using SaiTunnelTerm = SaiObject<SaiTunnelTermTraits>;

struct SaiTunnelHandle {
  std::shared_ptr<SaiTunnel> tunnel;
  std::shared_ptr<SaiTunnelTerm> tunnelTerm;
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

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  Handles handles_;
  sai_tunnel_type_t getSaiTunnelType(TunnelType type) {
    switch (type) {
      case IPINIP:
        return SAI_TUNNEL_TYPE_IPINIP;
      default:
        throw FbossError("Failed to convert IpTunnel type to SAI type: ", type);
    }
  }
  sai_tunnel_term_table_entry_type_t getSaiTunnelTermType(TunnelTermType type) {
    switch (type) {
      case MP2MP:
        return SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_MP2MP;
      default:
        throw FbossError(
            "Failed to convert IpTunnel term type to SAI type: ", type);
    }
  }
};

} // namespace facebook::fboss
