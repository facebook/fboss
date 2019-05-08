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

#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/MacAddress.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiRouterInterface;
class SaiVirtualRouter;

class SaiRouterInterface {
 public:
  SaiRouterInterface(
      SaiApiTable* apiTable,
      const RouterInterfaceApiParameters::Attributes& attributes,
      const sai_object_id_t& switchID);
  ~SaiRouterInterface();
  SaiRouterInterface(const SaiRouterInterface& other) = delete;
  SaiRouterInterface(SaiRouterInterface&& other) = delete;
  SaiRouterInterface& operator=(const SaiRouterInterface& other) = delete;
  SaiRouterInterface& operator=(SaiRouterInterface&& other) = delete;
  bool operator==(const SaiRouterInterface& other) const;
  bool operator!=(const SaiRouterInterface& other) const;

  const RouterInterfaceApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  RouterInterfaceApiParameters::Attributes attributes_;
  sai_object_id_t id_;
};

class SaiRouterInterfaceManager {
 public:
  SaiRouterInterfaceManager(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable);
  sai_object_id_t addRouterInterface(
      const std::shared_ptr<Interface>& swInterface);
  void removeRouterInterface(const InterfaceID& swId);
  void changeRouterInterface(
      const std::shared_ptr<Interface>& oldInterface,
      const std::shared_ptr<Interface>& newInterface);
  SaiRouterInterface* getRouterInterface(const InterfaceID& swId);
  const SaiRouterInterface* getRouterInterface(const InterfaceID& swId) const;

  void processInterfaceDelta(const StateDelta& stateDelta);

 private:
  SaiRouterInterface* getRouterInterfaceImpl(const InterfaceID& swId) const;
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  std::unordered_map<InterfaceID, std::unique_ptr<SaiRouterInterface>>
      routerInterfaces_;
};

} // namespace fboss
} // namespace facebook
