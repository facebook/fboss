/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

SaiVirtualRouterManager::SaiVirtualRouterManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
  if (platform_->getAsic()->isSupported(HwAsic::Feature::VRF)) {
    auto& store = saiStore_->get<SaiVirtualRouterTraits>();
    auto virtualRouterHandle = std::make_unique<SaiVirtualRouterHandle>();
    SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
    VirtualRouterSaiId defaultVrfId{
        SaiApiTable::getInstance()->switchApi().getAttribute(
            switchId, SaiSwitchTraits::Attributes::DefaultVirtualRouterId{})};
    virtualRouterHandle->virtualRouter = store.loadObjectOwnedByAdapter(
        SaiVirtualRouterTraits::AdapterKey{defaultVrfId});
    virtualRouterHandle->mplsRouterInterface =
        createMplsRouterInterface(defaultVrfId);

    CHECK(virtualRouterHandle->virtualRouter);
    handles_.emplace(
        std::make_pair(RouterID(0), std::move(virtualRouterHandle)));
  }
}
VirtualRouterSaiId SaiVirtualRouterManager::addVirtualRouter(
    const RouterID& /* routerId */) {
  throw FbossError("Adding new virtual routers is not supported");
  folly::assume_unreachable();
}

SaiVirtualRouterHandle* SaiVirtualRouterManager::getVirtualRouterHandle(
    const RouterID& routerId) {
  return getVirtualRouterHandleImpl(routerId);
}

const SaiVirtualRouterHandle* SaiVirtualRouterManager::getVirtualRouterHandle(
    const RouterID& routerId) const {
  return getVirtualRouterHandleImpl(routerId);
}

SaiVirtualRouterHandle* SaiVirtualRouterManager::getVirtualRouterHandleImpl(
    const RouterID& routerId) const {
  auto itr = handles_.find(routerId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second) {
    XLOG(FATAL) << "invalid null virtual router for routerID: " << routerId;
  }
  return itr->second.get();
}

std::shared_ptr<SaiMplsRouterInterface>
SaiVirtualRouterManager::createMplsRouterInterface(VirtualRouterSaiId vrId) {
  auto asicType = platform_->getAsic()->getAsicType();

  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
      return saiStore_->get<SaiMplsRouterInterfaceTraits>().setObject(
          SaiMplsRouterInterfaceTraits::AdapterHostKey{vrId},
          SaiMplsRouterInterfaceTraits::CreateAttributes{
              vrId, SAI_ROUTER_INTERFACE_TYPE_MPLS_ROUTER});

    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      // TODO(pshaikh): mpls support required
      break;
  }
  return nullptr;
}
} // namespace facebook::fboss
