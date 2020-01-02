// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmTrunk.h"
#include "fboss/agent/hw/bcm/PortAndEgressIdsMap.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

#include <folly/SpinLock.h>

extern "C" {
#include <opennsl/l3.h>
#include <opennsl/types.h>
}

namespace facebook::fboss {

class BcmEgressManager {
 public:
  using EgressIdSet = BcmEcmpEgress::EgressIdSet;

  explicit BcmEgressManager(const BcmSwitchIf* hw) : hw_(hw) {
    auto port2EgressIds = std::make_shared<PortAndEgressIdsMap>();
    port2EgressIds->publish();
    setPort2EgressIdsInternal(port2EgressIds);
  }
  /*
   * Port down handling
   * Look up egress entries going over this port and
   * then remove these from ecmp entries.
   * This is called from the linkscan callback and
   * we don't acquire BcmSwitch::lock_ here. See note above
   * declaration of BcmSwitch::linkStateChangedHwNotLocked which
   * explains why we can't hold this lock here.
   */
  void linkDownHwNotLocked(opennsl_port_t port) {
    linkStateChangedMaybeLocked(
        BcmPort::asGPort(port), false /*down*/, false /*not locked*/);
  }
  void trunkDownHwNotLocked(opennsl_trunk_t trunk) {
    linkStateChangedMaybeLocked(
        BcmTrunk::asGPort(trunk), false /*down*/, false /*not locked*/);
  }
  void linkDownHwLocked(opennsl_port_t port) {
    // Just call the non locked counterpart here.
    // We don't really need the lock for link down
    // handling
    linkDownHwNotLocked(port);
  }
  /*
   * link up handling. Only ever called
   * while holding hw lock.
   */
  void linkUpHwLocked(opennsl_port_t port) {
    linkStateChangedMaybeLocked(
        BcmPort::asGPort(port), true /*up*/, true /*locked*/);
  }
  /*
   * Update port to egressIds mapping
   */
  void updatePortToEgressMapping(
      opennsl_if_t egressId,
      opennsl_port_t oldPort,
      opennsl_port_t newPort);
  /*
   * Get port -> egressIds map
   */
  std::shared_ptr<PortAndEgressIdsMap> getPortAndEgressIdsMap() const {
    folly::SpinLockGuard guard(portAndEgressIdsLock_);
    return portAndEgressIdsDontUseDirectly_;
  }

  bool isResolved(const opennsl_if_t egressId) const {
    return resolvedEgresses_.find(egressId) != resolvedEgresses_.end();
  }
  void resolved(const opennsl_if_t egressId) {
    resolvedEgresses_.insert(egressId);
  }
  void unresolved(const opennsl_if_t egressId) {
    resolvedEgresses_.erase(egressId);
  }

 private:
  /*
   * Called both while holding and not holding the hw lock.
   */
  void linkStateChangedMaybeLocked(opennsl_port_t port, bool up, bool locked);
  static void egressResolutionChangedHwNotLocked(
      int unit,
      const EgressIdSet& affectedEgressIds,
      bool up);
  // Callback for traversal in egressResolutionChangedHwNotLocked
  static int removeAllEgressesFromEcmpCallback(
      int unit,
      opennsl_l3_egress_ecmp_t* ecmp, // ecmp object being traversed
      int intfCount, // number of egresses in the ecmp group
      opennsl_if_t* intfArray, // array of egresses in the ecmp group
      void* userData); // egresses we intend to remove from the ecmp group
  void setPort2EgressIdsInternal(std::shared_ptr<PortAndEgressIdsMap> newMap);

  const BcmSwitchIf* hw_;
  /*
   * The current port -> egressIds map.
   *
   * BEWARE: You generally shouldn't access this directly, even internally
   * within this class's private methods.  This should only be accessed while
   * holding port2EgressIdsLock_.  You almost certainly should call
   * getPort2EgressIds() or setPort2EgressIdsInternal() instead of directly
   * accessing this.
   *
   * This intentionally has an awkward name so people won't forget and try to
   * directly access this pointer.
   */
  std::shared_ptr<PortAndEgressIdsMap> portAndEgressIdsDontUseDirectly_;
  mutable folly::SpinLock portAndEgressIdsLock_;
  boost::container::flat_set<opennsl_if_t> resolvedEgresses_;
};

} // namespace facebook::fboss
