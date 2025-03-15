/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiArsProfileManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

#include <algorithm>

namespace facebook::fboss {

SaiNextHopGroupManager::SaiNextHopGroupManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiNextHopGroupHandle>
SaiNextHopGroupManager::incRefOrAddNextHopGroup(
    const RouteNextHopEntry::NextHopSet& swNextHops) {
  auto ins = handles_.refOrEmplace(swNextHops);
  std::shared_ptr<SaiNextHopGroupHandle> nextHopGroupHandle = ins.first;
  if (!ins.second) {
    return nextHopGroupHandle;
  }
  SaiNextHopGroupTraits::AdapterHostKey nextHopGroupAdapterHostKey;
  // Populate the set of rifId, IP pairs for the NextHopGroup's
  // AdapterHostKey, and a set of next hop ids to create members for
  // N.B.: creating a next hop group member relies on the next hop group
  // already existing, so we cannot create them inline in the loop (since
  // creating the next hop group requires going through all the next hops
  // to figure out the AdapterHostKey)
  for (const auto& swNextHop : swNextHops) {
    // Compute the sai id of the next hop's router interface
    InterfaceID interfaceId = swNextHop.intf();
    auto routerInterfaceHandle =
        managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
            interfaceId);
    if (!routerInterfaceHandle) {
      throw FbossError("Missing SAI router interface for ", interfaceId);
    }
    auto nhk = managerTable_->nextHopManager().getAdapterHostKey(
        folly::poly_cast<ResolvedNextHop>(swNextHop));
    nextHopGroupAdapterHostKey.insert(std::make_pair(nhk, swNextHop.weight()));
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  std::optional<SaiNextHopGroupTraits::Attributes::ArsObjectId> arsObjectId{
      std::nullopt};
  if (FLAGS_flowletSwitchingEnable &&
      platform_->getAsic()->isSupported(HwAsic::Feature::FLOWLET)) {
    auto arsHandlePtr = managerTable_->arsManager().getArsHandle();
    if (arsHandlePtr->ars) {
      auto arsSaiId = arsHandlePtr->ars->adapterKey();
      if (!managerTable_->arsManager().isFlowsetTableFull(arsSaiId)) {
        arsObjectId = SaiNextHopGroupTraits::Attributes::ArsObjectId{arsSaiId};
      }
    }
  }
#endif

  // Create the NextHopGroup and NextHopGroupMembers
  auto& store = saiStore_->get<SaiNextHopGroupTraits>();
  SaiNextHopGroupTraits::CreateAttributes nextHopGroupAttributes{
      SAI_NEXT_HOP_GROUP_TYPE_ECMP
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      ,
      arsObjectId
#endif
  };
  nextHopGroupHandle->nextHopGroup =
      store.setObject(nextHopGroupAdapterHostKey, nextHopGroupAttributes);
  NextHopGroupSaiId nextHopGroupId =
      nextHopGroupHandle->nextHopGroup->adapterKey();
  nextHopGroupHandle->fixedWidthMode = isFixedWidthNextHopGroup(swNextHops);
  nextHopGroupHandle->saiStore_ = saiStore_;
  nextHopGroupHandle->maxVariableWidthEcmpSize =
      platform_->getAsic()->getMaxVariableWidthEcmpSize();
  XLOG(DBG2) << "Created NexthopGroup OID: " << nextHopGroupId;

  for (const auto& swNextHop : swNextHops) {
    auto resolvedNextHop = folly::poly_cast<ResolvedNextHop>(swNextHop);
    auto managedNextHop =
        managerTable_->nextHopManager().addManagedSaiNextHop(resolvedNextHop);
    auto key = std::make_pair(nextHopGroupId, resolvedNextHop);
    auto weight = (resolvedNextHop.weight() == ECMP_WEIGHT)
        ? 1
        : resolvedNextHop.weight();
    auto result = nextHopGroupMembers_.refOrEmplace(
        key,
        this,
        nextHopGroupHandle.get(),
        nextHopGroupId,
        managedNextHop,
        weight,
        nextHopGroupHandle->fixedWidthMode);
    nextHopGroupHandle->members_.push_back(result.first);
  }
  return nextHopGroupHandle;
}

bool SaiNextHopGroupManager::isFixedWidthNextHopGroup(
    const RouteNextHopEntry::NextHopSet& swNextHops) const {
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::WIDE_ECMP)) {
    return false;
  }
  auto totalWeight = 0;
  for (const auto& swNextHop : swNextHops) {
    auto weight = swNextHop.weight() == ECMP_WEIGHT ? 1 : swNextHop.weight();
    totalWeight += weight;
  }
  if (totalWeight > platform_->getAsic()->getMaxVariableWidthEcmpSize()) {
    return true;
  }
  return false;
}

std::shared_ptr<SaiNextHopGroupMember> SaiNextHopGroupManager::createSaiObject(
    const typename SaiNextHopGroupMemberTraits::AdapterHostKey& key,
    const typename SaiNextHopGroupMemberTraits::CreateAttributes& attributes) {
  auto& store = saiStore_->get<SaiNextHopGroupMemberTraits>();
  return store.setObject(key, attributes);
}

std::shared_ptr<SaiNextHopGroupMember> SaiNextHopGroupManager::getSaiObject(
    const typename SaiNextHopGroupMemberTraits::AdapterHostKey& key) {
  auto& store = saiStore_->get<SaiNextHopGroupMemberTraits>();
  return store.get(key);
}

std::shared_ptr<SaiNextHopGroupMember>
SaiNextHopGroupManager::getSaiObjectFromWBCache(
    const typename SaiNextHopGroupMemberTraits::AdapterHostKey& key) {
  auto& store = saiStore_->get<SaiNextHopGroupMemberTraits>();
  return store.getWarmbootHandle(key);
}

// Convert all nexthops to ARS mode
// Conversion depends on availability in flowset table which is of size 32k
// If a nexthop is already in ARS mode, set operation is NOP
void SaiNextHopGroupManager::updateArsModeAll(
    const std::shared_ptr<FlowletSwitchingConfig>& newFlowletConfig) {
  if (!FLAGS_flowletSwitchingEnable ||
      !platform_->getAsic()->isSupported(HwAsic::Feature::FLOWLET)) {
    return;
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  auto arsHandlePtr = managerTable_->arsManager().getArsHandle();
  CHECK(arsHandlePtr->ars);
  auto arsSaiId = arsHandlePtr->ars->adapterKey();

  for (auto entry : handles_) {
    auto handle = entry.second;
    auto handlePtr = handle.lock();
    if (!handlePtr) {
      continue;
    }

    if (newFlowletConfig) {
      if (!managerTable_->arsManager().isFlowsetTableFull(arsSaiId)) {
        handlePtr->nextHopGroup->setOptionalAttribute(
            SaiNextHopGroupTraits::Attributes::ArsObjectId{arsSaiId});
      }
    } else {
      // flowlet config removal scenario
      handlePtr->nextHopGroup->setOptionalAttribute(
          SaiNextHopGroupTraits::Attributes::ArsObjectId{SAI_NULL_OBJECT_ID});
    }
  }
#endif
}

std::string SaiNextHopGroupManager::listManagedObjects() const {
  std::set<std::string> outputs{};
  for (auto entry : handles_) {
    auto handle = entry.second;
    auto handlePtr = handle.lock();
    if (!handlePtr) {
      continue;
    }
    std::string output{};
    for (auto member : handlePtr->members_) {
      output += member->toString();
    }
    outputs.insert(output);
  }

  std::string finalOutput{};
  for (auto output : outputs) {
    finalOutput += output;
    finalOutput += "\n";
  }
  return finalOutput;
}

NextHopGroupMember::NextHopGroupMember(
    SaiNextHopGroupManager* manager,
    SaiNextHopGroupHandle* nhgroup,
    SaiNextHopGroupTraits::AdapterKey nexthopGroupId,
    ManagedSaiNextHop managedSaiNextHop,
    NextHopWeight nextHopWeight,
    bool fixedWidthMode) {
  std::visit(
      [=, this](auto managedNextHop) {
        using ObjectTraits = typename std::decay_t<
            decltype(managedNextHop)>::element_type::ObjectTraits;
        auto key = managedNextHop->adapterHostKey();
        std::ignore = key;
        using ManagedMemberType = ManagedSaiNextHopGroupMember<ObjectTraits>;
        auto managedMember = std::make_shared<ManagedMemberType>(
            manager,
            nhgroup,
            managedNextHop,
            nexthopGroupId,
            nextHopWeight,
            fixedWidthMode);
        SaiObjectEventPublisher::getInstance()->get<ObjectTraits>().subscribe(
            managedMember);
        managedNextHopGroupMember_ = managedMember;
      },
      managedSaiNextHop);
}

template <typename NextHopTraits>
void ManagedSaiNextHopGroupMember<NextHopTraits>::createObject(
    typename ManagedSaiNextHopGroupMember<NextHopTraits>::PublisherObjects
        added) {
  CHECK(this->allPublishedObjectsAlive()) << "next hops are not ready";

  auto nexthopId = std::get<NextHopWeakPtr>(added).lock()->adapterKey();

  SaiNextHopGroupMemberTraits::AdapterHostKey adapterHostKey{
      nexthopGroupId_, nexthopId};
  // In fixed width case, the member is added with weight 0
  // and proper weight is set through bulk set api. check comments
  // associated with SaiNextHopGroupHandle::bulkProgramMembers for details
  SaiNextHopGroupMemberTraits::CreateAttributes createAttributes{
      nexthopGroupId_, nexthopId, fixedWidthMode_ ? 0 : weight_};

  bool bulkUpdate{true};
  if (fixedWidthMode_) {
    // In fixed width case, do not recreate member with 0 weight
    // if it already exists.
    auto existingObj = manager_->getSaiObject(adapterHostKey);
    if (existingObj) {
      createAttributes = existingObj->attributes();
      // For warmboot, avoid bulk set as all members may not
      // be active yet.
      if (manager_->getSaiObjectFromWBCache(adapterHostKey)) {
        bulkUpdate = false;
      }
    }
  }

  auto object = manager_->createSaiObject(adapterHostKey, createAttributes);
  this->setObject(object);
  if (fixedWidthMode_) {
    // notify nhgroup to bulk program correct weight
    nhgroup_->memberAdded({adapterHostKey, weight_}, bulkUpdate);
  }
  XLOG(DBG2) << "ManagedSaiNextHopGroupMember::createObject: " << toString()
             << " weight " << weight_.value();
}

template <typename NextHopTraits>
void ManagedSaiNextHopGroupMember<NextHopTraits>::removeObject(
    size_t /* index */,
    typename ManagedSaiNextHopGroupMember<NextHopTraits>::PublisherObjects
    /* removed */) {
  XLOG(DBG2) << "ManagedSaiNextHopGroupMember::removeObject: " << toString();
  if (fixedWidthMode_) {
    // notify nhgroup to bulk program with 0 weight. In fixed width mode
    // member cannot be removed directly. check comments associated with
    // SaiNextHopGroupHandle::bulkProgramMembers for details
    nhgroup_->memberRemoved({this->getObject()->adapterHostKey(), weight_});
  }
  /* remove nexthop group member if next hop is removed */
  this->resetObject();
}

size_t SaiNextHopGroupHandle::nextHopGroupSize() const {
  return std::count_if(
      std::begin(members_), std::end(members_), [](auto member) {
        return member->isProgrammed();
      });
}

void SaiNextHopGroupHandle::memberAdded(
    SaiNextHopGroupMemberInfo memberInfo,
    bool updateHardware) {
  bulkProgramMembers(memberInfo, true /* added */, updateHardware);
}

void SaiNextHopGroupHandle::memberRemoved(
    SaiNextHopGroupMemberInfo memberInfo,
    bool updateHardware) {
  bulkProgramMembers(memberInfo, false /* added */, updateHardware);
}

/*
 * Perform a bulk update of ecmp member weights.
 * Some ASICs support a fixed width ecmp mode in which the total weight
 * of ECMP members can be significantly higher than regular mode.
 * However the total member weight needs to fixed (eg 512)
 * for the life of the object. Hence members cannot be added or
 * removed individually from the nexthop group as it will result
 * in having less or more total weight than the original count.
 * In fixed width mode, member addition and deletion are performed
 * through the following sequence.
 * Member add:
 *     - Add new member to nexthop group with weight 0
 *     - Bulk set the weight of all members in group such that the
 *       total weight remains the same.
 * Member removal:
 *     - Bulk set the weight of member to be removed to 0 and the weights
 *       of remaining members such that total weight remains the same
 *     - Remove the member with weight 0 from nexthop group
 */
void SaiNextHopGroupHandle::bulkProgramMembers(
    SaiNextHopGroupMemberInfo modifiedMemberInfo,
    bool added,
    bool updateHardware) {
  if (added) {
    fixedWidthNextHopGroupMembers_.insert(modifiedMemberInfo);
  } else {
    if (fixedWidthNextHopGroupMembers_.find(modifiedMemberInfo) ==
        fixedWidthNextHopGroupMembers_.end()) {
      XLOG(DBG2) << "Cannot find member to delete for fixed width ecmp";
      return;
    }
    fixedWidthNextHopGroupMembers_.erase(modifiedMemberInfo);
  }
  if (!updateHardware) {
    return;
  }
  std::vector<uint64_t> memberWeights;
  uint64_t totalWeight{0};
  for (const auto& member : fixedWidthNextHopGroupMembers_) {
    const auto& weight = member.second.value();
    totalWeight += weight;
    memberWeights.emplace_back(weight);
  }
  // normalize the weights to ecmp width if needed
  if (totalWeight > maxVariableWidthEcmpSize) {
    RouteNextHopEntry::normalizeNextHopWeightsToMaxPaths(
        memberWeights, FLAGS_ecmp_width);
  }
  std::vector<SaiNextHopGroupMemberTraits::AdapterHostKey> adapterHostKeys;
  std::vector<SaiNextHopGroupMemberTraits::Attributes::Weight> weights;
  int idx = 0;
  for (const auto& member : fixedWidthNextHopGroupMembers_) {
    auto [adapterHostKey, weight] = member;
    adapterHostKeys.emplace_back(adapterHostKey);
    weights.emplace_back(memberWeights.at(idx++));
  }
  // For removed entry, set the weight to 0 before deleting
  // the member from hardware
  if (!added) {
    adapterHostKeys.emplace_back(modifiedMemberInfo.first);
    weights.emplace_back(0);
  }
  if (adapterHostKeys.size()) {
    auto& store = saiStore_->get<SaiNextHopGroupMemberTraits>();
    store.setObjects(adapterHostKeys, weights);
  }
}

SaiNextHopGroupHandle::~SaiNextHopGroupHandle() {
  if (fixedWidthMode) {
    std::vector<SaiNextHopGroupMemberTraits::AdapterHostKey> adapterHostKeys;
    for (const auto& member : fixedWidthNextHopGroupMembers_) {
      adapterHostKeys.emplace_back(member.first);
    }
    if (adapterHostKeys.size()) {
      // Bulk set member weights to 0 so that members can be removed from
      // group member destructor without violating fixed width restriction
      std::vector<SaiNextHopGroupMemberTraits::Attributes::Weight> weights(
          fixedWidthNextHopGroupMembers_.size(), 0);
      auto& store = saiStore_->get<SaiNextHopGroupMemberTraits>();
      store.setObjects(adapterHostKeys, weights);
    }
  }
}

template <typename NextHopTraits>
std::string ManagedSaiNextHopGroupMember<NextHopTraits>::toString() const {
  auto nextHopGroupMemberIdStr = this->getObject()
      ? std::to_string(this->getObject()->adapterKey())
      : "none";
  return folly::to<std::string>(
      this->getObject() ? "active " : "inactive ",
      "managed nhg member: ",
      "NextHopGroupId: ",
      nexthopGroupId_,
      "NextHopGroupMemberId:",
      nextHopGroupMemberIdStr);
}

} // namespace facebook::fboss
