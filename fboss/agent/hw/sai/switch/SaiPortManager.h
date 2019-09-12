/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiPort = SaiObject<SaiPortTraits>;

struct SaiPortHandle {
  std::shared_ptr<SaiPort> port;
  std::shared_ptr<SaiBridgePort> bridgePort;
  folly::F14FastMap<uint8_t, std::shared_ptr<SaiQueue>> queues;
};

class SaiPortManager {
 public:
  SaiPortManager(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);
  PortSaiId addPort(const std::shared_ptr<Port>& swPort);
  void removePort(PortID id);
  void changePort(const std::shared_ptr<Port>& swPort);

  SaiPortTraits::CreateAttributes attributesFromSwPort(
      const std::shared_ptr<Port>& swPort) const;
  const SaiPortHandle* getPortHandle(PortID swId) const;
  SaiPortHandle* getPortHandle(PortID swId);
  PortID getPortID(sai_object_id_t saiId) const;
  void processPortDelta(const StateDelta& stateDelta);

 private:
  SaiPortHandle* getPortHandleImpl(PortID swId) const;
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  folly::F14FastMap<PortID, std::unique_ptr<SaiPortHandle>> handles_;
  folly::F14FastMap<sai_object_id_t, PortID> portSaiIds_;
};

} // namespace fboss
} // namespace facebook
