// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

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
  folly::SpinLockGuard guard(portAndEgressIdsLock_);
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
        hw_->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER));
  }
}

template <typename T>
BcmEcmpEgress::EgressId BcmEgressManager::toEgressId(T egress) {
  return egress;
}

#ifdef BCM_L3_ECMP_MEMBER_WEIGHTED
template <>
BcmEcmpEgress::EgressId BcmEgressManager::toEgressId<bcm_l3_ecmp_member_t>(
    bcm_l3_ecmp_member_t egress) {
  return egress.egress_if;
}
#endif

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
  EgressIdSet* egressesToRemove = static_cast<EgressIdSet*>(userData);
  for (int i = 0; i < memberCount; ++i) {
    BcmEcmpEgress::EgressId egressInHw = toEgressId<T>(memberArray[i]);
    if (egressesToRemove->find(egressInHw) != egressesToRemove->end()) {
      BcmEcmpEgress::removeEgressIdHwNotLocked(
          unit, ecmp->ecmp_intf, egressInHw);
    }
  }
  return 0;
}

void BcmEgressManager::egressResolutionChangedHwNotLocked(
    int unit,
    const EgressIdSet& affectedEgressIds,
    bool up,
    bool weightedMember) {
  CHECK(!up);
  EgressIdSet tmpEgressIds(affectedEgressIds);
  if (weightedMember) {
#ifdef BCM_L3_ECMP_MEMBER_WEIGHTED
    bcm_l3_ecmp_traverse(
        unit,
        removeAllEgressesFromEcmpCallback<bcm_l3_ecmp_member_t>,
        &tmpEgressIds);
#endif
  } else {
    bcm_l3_egress_ecmp_traverse(
        unit, removeAllEgressesFromEcmpCallback<bcm_if_t>, &tmpEgressIds);
  }
}

} // namespace facebook::fboss
