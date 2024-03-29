// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <bcm/l3.h>
#include <bcm/types.h>
}

#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmNextHop.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

namespace facebook::fboss {

/**
 * Class to abstract ECMP path
 *
 * There are 2 use cases for BCM Ecmp host
 * a) As a collection of BcmHost entries - Unlike BcmHost, in this case
 * BcmMultiPathNextHop does not have its own HW programming. It functions as
 * a SW object which refers to one or multiple BcmHost objects.
 * b) As a object representing a host route. In this case the
 * BcmMultiPathNextHop simply references another egress entry (which maybe
 * either BcmEgress or BcmEcmpEgress).
 */
using BcmMultiPathNextHopKey = std::pair<bcm_vrf_t, RouteNextHopSet>;

class BcmNextHop;

class BcmMultiPathNextHop {
 public:
  BcmMultiPathNextHop(const BcmSwitchIf* hw, BcmMultiPathNextHopKey key);
  virtual ~BcmMultiPathNextHop();
  bcm_if_t getEgressId() const;
  bcm_if_t getEcmpEgressId() const {
    return ecmpEgress_ ? ecmpEgress_->getID() : BcmEgressBase::INVALID;
  }

  BcmEcmpEgress* getEgress() const {
    return ecmpEgress_.get();
  }

 private:
  std::shared_ptr<BcmNextHop> refOrEmplaceNextHop(const HostKey& key);

  const BcmSwitchIf* hw_;
  bcm_vrf_t vrf_;
  RouteNextHopSet fwd_;
  std::vector<std::shared_ptr<BcmNextHop>> nexthops_;
  std::unique_ptr<BcmEcmpEgress> ecmpEgress_;
  BcmMultiPathNextHopKey key_;
};

using BcmMultiPathNextHopTableBase =
    BcmNextHopTable<BcmMultiPathNextHopKey, BcmMultiPathNextHop>;
class BcmMultiPathNextHopTable : public BcmMultiPathNextHopTableBase {
 public:
  using EgressIdSet = BcmEcmpEgress::EgressIdSet;
  explicit BcmMultiPathNextHopTable(BcmSwitch* hw)
      : BcmMultiPathNextHopTableBase(hw) {
    ucmpSupported_ = getBcmSwitch()->getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER);
    useHsdk_ = getBcmSwitch()->getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::HSDK);
    wideEcmpSupported_ = getBcmSwitch()->getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::WIDE_ECMP);
  }
  void egressResolutionChangedHwLocked(
      const EgressIdSet& affectedEgressIds,
      BcmEcmpEgress::Action action);
  void egressResolutionChangedHwLocked(
      bcm_if_t affectedPath,
      BcmEcmpEgress::Action action) {
    EgressIdSet affectedEgressIds;
    affectedEgressIds.insert(affectedPath);
    egressResolutionChangedHwLocked(affectedEgressIds, action);
  }

  long getEcmpEgressCount() const;
  void updateEcmpsForFlowletSwitching();
  bool updateEcmpsForFlowletTableLocked();

 private:
  bool ucmpSupported_{false};
  bool useHsdk_{false};
  bool wideEcmpSupported_{false};
};

class BcmMultiPathNextHopStatsManager {
 public:
  BcmMultiPathNextHopStatsManager() = default;
  ~BcmMultiPathNextHopStatsManager() = default;

  void addBcmMultiPathNextHopKey(
      const BcmMultiPathNextHopKey& key,
      const std::weak_ptr<BcmMultiPathNextHop>& wp) {
    bcmMultiPathNextHopKeys_.wlock()->emplace(key, wp);
  }

  void removeBcmMultiPathNextHopKey(BcmMultiPathNextHopKey& key) {
    bcmMultiPathNextHopKeys_.wlock()->erase(key);
  }

  std::map<BcmMultiPathNextHopKey, std::weak_ptr<BcmMultiPathNextHop>>
  getBcmMultiPathNextHopKeysExpensive() const {
    auto keys = bcmMultiPathNextHopKeys_.rlock();
    return *keys;
  }

  std::vector<EcmpDetails> getAllEcmpDetails() const;
  HwFlowletStats getHwFlowletStats() const;

 private:
  // map of all the bcm multipath nexthop keys and weak ptr
  folly::Synchronized<
      std::map<BcmMultiPathNextHopKey, std::weak_ptr<BcmMultiPathNextHop>>>
      bcmMultiPathNextHopKeys_;
};
} // namespace facebook::fboss
