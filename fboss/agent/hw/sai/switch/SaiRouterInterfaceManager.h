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
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/MacAddress.h"

#include "folly/Overload.h"
#include "folly/container/F14Map.h"

#include <memory>
#include <mutex>
#include <variant>
#include <vector>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiVlanRouterInterface = SaiObject<SaiVlanRouterInterfaceTraits>;
using SaiPortRouterInterface = SaiObject<SaiPortRouterInterfaceTraits>;

struct SaiRouterInterfaceHandle {
  using SaiRouterInterface = std::variant<
      std::shared_ptr<SaiVlanRouterInterface>,
      std::shared_ptr<SaiPortRouterInterface>>;
  SaiRouterInterface routerInterface;
  SaiRouterInterfaceHandle(cfg::InterfaceType type) : intfType(type) {}
  RouterInterfaceSaiId adapterKey() const {
    return std::visit(
        [](auto& handle) { return handle->adapterKey(); }, routerInterface);
  }
  cfg::InterfaceType type() const {
    return intfType;
  }
  void setLocal(bool isLocal) {
    isLocalRif = isLocal;
  }
  bool isLocal() const {
    return isLocalRif;
  }
  std::shared_ptr<SaiPortRouterInterface> getPortRouterInterface() {
    return std::get<std::shared_ptr<SaiPortRouterInterface>>(routerInterface);
  }

  std::shared_ptr<SaiVlanRouterInterface> getVlanRouterInterface() {
    return std::get<std::shared_ptr<SaiVlanRouterInterface>>(routerInterface);
  }

  std::vector<std::shared_ptr<SaiRoute>> toMeRoutes;
  bool isLocalRif{true};
  cfg::InterfaceType intfType{cfg::InterfaceType::VLAN};
};

class SaiRouterInterfaceManager {
 public:
  SaiRouterInterfaceManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  RouterInterfaceSaiId addLocalRouterInterface(
      const std::shared_ptr<Interface>& swInterface) {
    return addRouterInterface(swInterface, true /*isLocal*/);
  }
  void removeLocalRouterInterface(
      const std::shared_ptr<Interface>& swInterface) {
    removeRouterInterface(swInterface);
  }
  void changeLocalRouterInterface(
      const std::shared_ptr<Interface>& oldInterface,
      const std::shared_ptr<Interface>& newInterface) {
    changeRouterInterface(oldInterface, newInterface, true /*isLocal*/);
  }

  RouterInterfaceSaiId addRemoteRouterInterface(
      const std::shared_ptr<Interface>& swInterface) {
    return addRouterInterface(swInterface, false /*remote*/);
  }
  void removeRemoteRouterInterface(
      const std::shared_ptr<Interface>& swInterface) {
    removeRouterInterface(swInterface);
  }
  void changeRemoteRouterInterface(
      const std::shared_ptr<Interface>& oldInterface,
      const std::shared_ptr<Interface>& newInterface) {
    changeRouterInterface(oldInterface, newInterface, false /*remote*/);
  }

  SaiRouterInterfaceHandle* getRouterInterfaceHandle(const InterfaceID& swId);
  const SaiRouterInterfaceHandle* getRouterInterfaceHandle(
      const InterfaceID& swId) const;

  std::optional<InterfaceID> getRouterPortInterfaceIDIf(PortID port) const;

  std::optional<InterfaceID> getRouterPortInterfaceIDIf(PortSaiId port) const;

 private:
  RouterInterfaceSaiId addRouterInterface(
      const std::shared_ptr<Interface>& swInterface,
      bool isLocal);
  void removeRouterInterface(const std::shared_ptr<Interface>& swInterface);
  void changeRouterInterface(
      const std::shared_ptr<Interface>& oldInterface,
      const std::shared_ptr<Interface>& newInterface,
      bool isLocal);
  RouterInterfaceSaiId addOrUpdateRouterInterface(
      const std::shared_ptr<Interface>& swInterface,
      bool isLocal);
  RouterInterfaceSaiId addOrUpdateVlanRouterInterface(
      const std::shared_ptr<Interface>& swInterface,
      bool isLocal);
  RouterInterfaceSaiId addOrUpdatePortRouterInterface(
      const std::shared_ptr<Interface>& swInterface,
      bool isLocal);
  SaiRouterInterfaceHandle* getRouterInterfaceHandleImpl(
      const InterfaceID& swId) const;
  SaiPortRouterInterfaceTraits::Attributes::PortId getPortId(
      const std::shared_ptr<Interface>& swInterface);
  SaiPortRouterInterfaceTraits::Attributes::PortId getSystemPortId(
      const std::shared_ptr<Interface>& swInterface);

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::F14FastMap<InterfaceID, std::unique_ptr<SaiRouterInterfaceHandle>>
      handles_;
};

} // namespace facebook::fboss
