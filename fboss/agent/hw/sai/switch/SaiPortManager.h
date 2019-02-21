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
#include "fboss/agent/state/Port.h"
#include "fboss/agent/types.h"

#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiBridgePort;
class SaiManagerTable;

class SaiPort {
 public:
  SaiPort(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const PortApiParameters::Attributes& attributes);
  ~SaiPort();
  // This is an RAII type which manages the lifetime of the port
  // being programmed to the underlying SAI Adapter. For this reason,
  // we cannot copy it. We could move it, but we choose to just
  // store it in a unique_ptr to accomplish that.
  SaiPort(const SaiPort& other) = delete;
  SaiPort(SaiPort&& other) = delete;
  SaiPort& operator=(const SaiPort& other) = delete;
  SaiPort& operator=(SaiPort&& other) = delete;
  bool operator==(const SaiPort& other) const;
  bool operator!=(const SaiPort& other) const;

  void setAttributes(const PortApiParameters::Attributes& desiredAttributes);
  const PortApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }
  const SaiBridgePort* getBridgePort() const {
    return bridgePort_.get();
  }

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  PortApiParameters::Attributes attributes_;
  sai_object_id_t id_;
  std::unique_ptr<SaiBridgePort> bridgePort_;
};

class SaiPortManager {
 public:
  SaiPortManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);
  sai_object_id_t addPort(const std::shared_ptr<Port>& swPort);
  void removePort(PortID id);
  void changePort(const std::shared_ptr<Port>& swPort);

  PortApiParameters::Attributes attributesFromSwPort(
      const std::shared_ptr<Port>& swPort) const;
  const SaiPort* getPort(PortID swId) const;
  SaiPort* getPort(PortID swId);

 private:
  SaiPort* getPortImpl(PortID swId) const;
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  std::unordered_map<PortID, std::unique_ptr<SaiPort>> ports_;
};

} // namespace fboss
} // namespace facebook
