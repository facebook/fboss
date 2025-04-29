// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace {
constexpr auto kDefaultMemberWeight = 1;
}

namespace facebook::fboss {

void BcmEgressManager::updatePortToEgressMapping(
    bcm_if_t egressId,
    bcm_gport_t oldGPort,
    bcm_gport_t newGPort) {
  auto newMapping = getPortAndEgressIdsMap()->clone();

  if (BcmPort::isValidLocalPort(oldGPort) ||
      BcmTrunk::isValidTrunkPort(oldGPort)) {
    auto old = newMapping->getPortAndEgressIdsIf(oldGPort);
    CHECK(old);
    auto oldCloned = old->clone();
    oldCloned->removeEgressId(egressId);
    if (oldCloned->empty()) {
      newMapping->removePort(oldGPort);
    } else {
      newMapping->updatePortAndEgressIds(oldCloned);
    }
  }
  if (BcmPort::isValidLocalPort(newGPort) ||
      BcmTrunk::isValidTrunkPort(newGPort)) {
    auto existing = newMapping->getPortAndEgressIdsIf(newGPort);
    if (existing) {
      auto existingCloned = existing->clone();
      existingCloned->addEgressId(egressId);
      newMapping->updatePortAndEgressIds(existingCloned);
    } else {
      PortAndEgressIdsFields::EgressIdSet egressIds;
      egressIds.insert(egressId);
      auto toAdd = std::make_shared<PortAndEgressIds>(newGPort, egressIds);
      newMapping->addPortAndEgressIds(toAdd);
    }
  }
  // Publish and replace with the updated mapping
  newMapping->publish();
  setPort2EgressIdsInternal(newMapping);
}

void BcmEgressManager::setPort2EgressIdsInternal(
    std::shared_ptr<PortAndEgressIdsMap> newMap) {
  // This is one of the only two places that should ever directly access
  // portAndEgressIdsDontUseDirectly_.  (getPortAndEgressIdsMap() being the
  // other one.)
  CHECK(newMap->isPublished());
  std::unique_lock guard(portAndEgressIdsLock_);
  portAndEgressIdsDontUseDirectly_.swap(newMap);
}

void BcmEgressManager::linkStateChangedMaybeLocked(
    bcm_gport_t gport,
    bool up,
    bool locked) {
  auto portAndEgressIdMapping = getPortAndEgressIdsMap();

  const auto portAndEgressIds =
      portAndEgressIdMapping->getPortAndEgressIdsIf(gport);
  if (!portAndEgressIds) {
    return;
  }
  if (locked) {
    hw_->writableMultiPathNextHopTable()->egressResolutionChangedHwLocked(
        portAndEgressIds->getEgressIds(),
        up ? BcmEcmpEgress::Action::EXPAND : BcmEcmpEgress::Action::SHRINK);
  } else {
    CHECK(!up);
    egressResolutionChangedHwNotLocked(
        hw_->getUnit(),
        portAndEgressIds->getEgressIds(),
        up,
        hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::WIDE_ECMP),
        hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HSDK));
  }
}

template <typename T>
BcmEgressManager::EgressIdAndWeight BcmEgressManager::toEgressIdAndWeight(
    T egress) {
  return std::make_pair(egress, kDefaultMemberWeight);
}

template <>
BcmEgressManager::EgressIdAndWeight
BcmEgressManager::toEgressIdAndWeight<bcm_l3_ecmp_member_t>(
    bcm_l3_ecmp_member_t egress) {
  return std::make_pair(egress.egress_if, egress.weight);
}

template <typename T>
int BcmEgressManager::removeAllEgressesFromEcmpCallback(
    int unit,
    bcm_l3_egress_ecmp_t* ecmp,
    int memberCount,
    T* memberArray,
    void* userData) {
  // loop over the egresses present in the ecmp group. For each that is
  // in the passed in list of egresses to remove (userData),
  // remove it from the ecmp group.
  auto tuple = *static_cast<std::tuple<EgressIdSet*, bool, bool>*>(userData);
  auto egressesToRemove = std::get<0>(tuple);
  auto useHsdk = std::get<1>(tuple);
  auto wideEcmpSupported = std::get<2>(tuple);
  auto ucmpEnabled = ecmp->ecmp_group_flags & BCM_L3_ECMP_MEMBER_WEIGHTED;
  for (int i = 0; i < memberCount; ++i) {
    auto egressInHw = toEgressIdAndWeight<T>(memberArray[i]);
    if (egressesToRemove->find(egressInHw.first) != egressesToRemove->end()) {
      BcmEcmpEgress::removeEgressIdHwNotLocked(
          unit,
          ecmp->ecmp_intf,
          ucmpEnabled ? egressInHw
                      : std::make_pair(egressInHw.first, kDefaultMemberWeight),
          ucmpEnabled,
          wideEcmpSupported,
          useHsdk);
    }
  }
  return 0;
}

void BcmEgressManager::egressResolutionChangedHwNotLocked(
    int unit,
    const EgressIdSet& affectedEgressIds,
    bool up,
    bool wideEcmpSupported,
    bool useHsdk) {
  CHECK(!up);
  EgressIdSet tmpEgressIds(affectedEgressIds);
  auto userData = std::make_tuple(&tmpEgressIds, useHsdk, wideEcmpSupported);
  if (useHsdk) {
    bcm_l3_ecmp_traverse(
        unit,
        removeAllEgressesFromEcmpCallback<bcm_l3_ecmp_member_t>,
        &userData);
  } else {
    bcm_l3_egress_ecmp_traverse(
        unit, removeAllEgressesFromEcmpCallback<bcm_if_t>, &userData);
  }
}

void BcmEgressManager::processFlowletSwitchingConfigChanged(
    const std::shared_ptr<FlowletSwitchingConfig>& newFlowletSwitching) {
  BcmFlowletConfig tmpFlowletConfig;
  if (newFlowletSwitching) {
    tmpFlowletConfig.inactivityIntervalUsecs =
        newFlowletSwitching->getInactivityIntervalUsecs();
    tmpFlowletConfig.flowletTableSize =
        newFlowletSwitching->getFlowletTableSize();
    tmpFlowletConfig.maxLinks = newFlowletSwitching->getMaxLinks();
    tmpFlowletConfig.switchingMode = newFlowletSwitching->getSwitchingMode();
  } else {
    tmpFlowletConfig.inactivityIntervalUsecs = 0;
    tmpFlowletConfig.flowletTableSize = 0;
    tmpFlowletConfig.maxLinks = 0;
    tmpFlowletConfig.switchingMode = cfg::SwitchingMode::FIXED_ASSIGNMENT;
  }
  // take the write lock
  *bcmFlowletConfig_.wlock() = tmpFlowletConfig;
  // update the all the ecmps in the hw
  // if the flowlet switching config changed
  hw_->writableMultiPathNextHopTable()->updateEcmpsForFlowletSwitching();
}

void BcmEgressManager::updateAllEgressForFlowletSwitching() {
  //  For TH4 port flowlet config is programmed in port object.
  //  Hence skipping the update of egress objects for TH4.
  //  TH3 needs update of all egress objects for port flowlet config changes.
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::ARS_PORT_ATTRIBUTES)) {
    return;
  }

  std::unordered_set<bcm_if_t> egressIds;
  for (const auto& hostTableEntry : *hw_->getHostTable()) {
    std::shared_ptr<BcmHostIf> host = hostTableEntry.second.lock();
    auto hostKey = hostTableEntry.first;
    auto bcmEgress = host->getEgress();
    if (!bcmEgress) {
      continue;
    }
    auto egressId = bcmEgress->getID();
    // skip programming same egress Id more than once
    if (!(egressIds.count(egressId)) && host->isProgrammedToNextHops()) {
      egressIds.insert(egressId);
      auto vrf = hostKey.getVrf();
      auto addr = hostKey.addr();
      auto mac = bcmEgress->getMac();
      auto intf = bcmEgress->getIntfId();
      auto gport = host->getSetPortAsGPort();
      auto port = BCM_GPORT_LOCAL_GET(gport);
      // skip for cpu port
      if (gport != BCM_GPORT_LOCAL_CPU) {
        bcmEgress->programToPort(intf, vrf, addr, mac, port);
      }
    }
  }
}
} // namespace facebook::fboss
