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
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

#include <algorithm>

namespace facebook::fboss {

bool isEcmpModeDynamic(std::optional<cfg::SwitchingMode> switchingMode) {
  return (
      switchingMode.has_value() &&
      (switchingMode.value() == cfg::SwitchingMode::PER_PACKET_QUALITY ||
       switchingMode.value() == cfg::SwitchingMode::FLOWLET_QUALITY));
}

SaiNextHopGroupManager::SaiNextHopGroupManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiNextHopGroupHandle>
SaiNextHopGroupManager::incRefOrAddNextHopGroup(const SaiNextHopGroupKey& key) {
  auto ins = handles_.refOrEmplace(key);
  std::shared_ptr<SaiNextHopGroupHandle> nextHopGroupHandle = ins.first;
  if (!ins.second) {
    return nextHopGroupHandle;
  }
  const auto& swNextHops = key.first;
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
    nextHopGroupAdapterHostKey.nhopMemberSet.insert(
        std::make_pair(nhk, swNextHop.weight()));
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  std::optional<SaiNextHopGroupTraits::Attributes::ArsObjectId> arsObjectId{
      std::nullopt};
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
  std::optional<SaiNextHopGroupTraits::Attributes::HashAlgorithm> hashAlgorithm{
      std::nullopt};
#endif

  if (FLAGS_flowletSwitchingEnable &&
      platform_->getAsic()->isSupported(HwAsic::Feature::ARS)) {
    auto overrideEcmpSwitchingMode = key.second;

    // if overrideEcmpSwitchingMode is empty, then use primary mode
    // if overrideEcmpSwitchingMode has value, then a backup mode is requested
    // by ERM
    nextHopGroupHandle->desiredArsMode_ = overrideEcmpSwitchingMode.has_value()
        ? overrideEcmpSwitchingMode
        : primaryArsMode_;

    if (isEcmpModeDynamic(nextHopGroupHandle->desiredArsMode_)) {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      auto arsHandlePtr = managerTable_->arsManager().getArsHandle();
      if (arsHandlePtr->ars) {
        auto arsSaiId = arsHandlePtr->ars->adapterKey();
        arsObjectId = SaiNextHopGroupTraits::Attributes::ArsObjectId{arsSaiId};
      }
#endif
    } else {
      if (nextHopGroupHandle->desiredArsMode_.has_value() &&
          (nextHopGroupHandle->desiredArsMode_.value() ==
           cfg::SwitchingMode::PER_PACKET_RANDOM)) {
        // setting hash algo to RANDOM is specific to TH* asics
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
        if (platform_->getAsic()->isSupported(
                HwAsic::Feature::SET_NEXT_HOP_GROUP_HASH_ALGORITHM)) {
          hashAlgorithm = SaiNextHopGroupTraits::Attributes::HashAlgorithm{
              SAI_HASH_ALGORITHM_RANDOM};
        }
#endif
      }
    }
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    nextHopGroupAdapterHostKey.mode =
        managerTable_->arsManager().cfgSwitchingModeToSai(
            nextHopGroupHandle->desiredArsMode_.value());
#endif
  }

  // Create the NextHopGroup and NextHopGroupMembers
  auto& store = saiStore_->get<SaiNextHopGroupTraits>();
  SaiNextHopGroupTraits::CreateAttributes nextHopGroupAttributes{
      SAI_NEXT_HOP_GROUP_TYPE_ECMP
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      ,
      arsObjectId
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
      ,
      hashAlgorithm
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
  nextHopGroupHandle->platform_ = platform_;

  XLOG(DBG2) << "Created NexthopGroup OID: " << nextHopGroupId;

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0) || defined(BRCM_SAI_SDK_XGS_GTE_13_0)
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::BULK_CREATE_ECMP_MEMBER)) {
    // TODO(zecheng): Use bulk create for warmboot handle reclaiming as well.
    // There is a sequencing issue where the delayed bulk create will cause
    // object removal during warmboot handle reclaiming. To workaround this,
    // disable bulk create during this process (as it is no-op in SDK).
    if (platform_->getHwSwitch()->getRunState() >= SwitchRunState::CONFIGURED &&
        !dynamic_cast<SaiSwitch*>(platform_->getHwSwitch())
             ->getRollbackInProgress_()) {
      nextHopGroupHandle->bulkCreate = true;
    }
  }

#endif

  for (const auto& swNextHop : swNextHops) {
    auto resolvedNextHop = folly::poly_cast<ResolvedNextHop>(swNextHop);
    auto managedNextHop =
        managerTable_->nextHopManager().addManagedSaiNextHop(resolvedNextHop);
    auto memberKey = std::make_pair(nextHopGroupId, resolvedNextHop);
    auto weight = (resolvedNextHop.weight() == ECMP_WEIGHT)
        ? 1
        : resolvedNextHop.weight();
    auto result = nextHopGroupMembers_.refOrEmplace(
        memberKey,
        this,
        nextHopGroupHandle.get(),
        nextHopGroupId,
        managedNextHop,
        weight,
        nextHopGroupHandle->fixedWidthMode);
    nextHopGroupHandle->members_.push_back(result.first);
  }

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0) || defined(BRCM_SAI_SDK_XGS_GTE_13_0)
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::BULK_CREATE_ECMP_MEMBER)) {
    nextHopGroupHandle->bulkCreate = false;

    std::vector<SaiNextHopGroupMemberTraits::AdapterHostKey> adapterHostKeys;
    std::vector<SaiNextHopGroupMemberTraits::CreateAttributes> createAttributes;

    // If next hop is not yet resolved, it will not have adapterHostKey and
    // create attributes.
    for (const auto& member : nextHopGroupHandle->members_) {
      const auto& [adapterHostKey, createAttribute] =
          member->getAdapterHostKeyAndCreateAttributes();
      CHECK_EQ(adapterHostKey.has_value(), createAttribute.has_value());
      if (adapterHostKey.has_value() && createAttribute.has_value()) {
        adapterHostKeys.push_back(adapterHostKey.value());
        createAttributes.push_back(createAttribute.value());
      }
    }
    if (!adapterHostKeys.empty()) {
      auto& store = saiStore_->get<SaiNextHopGroupMemberTraits>();
      auto objects = store.bulkCreateObjects(adapterHostKeys, createAttributes);
      CHECK_EQ(objects.size(), adapterHostKeys.size());

      auto iter = nextHopGroupHandle->members_.begin();
      for (int i = 0; i < adapterHostKeys.size(); i++) {
        while ((*iter)->getAdapterHostKeyAndCreateAttributes().first !=
               adapterHostKeys[i]) {
          iter++;
        }
        (*iter)->setObject(objects[i]);
        iter++;
      }
    }
  }
#endif

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
      !platform_->getAsic()->isSupported(HwAsic::Feature::ARS)) {
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

    // do not convert backup modes to dynamic
    if (!isEcmpModeDynamic(handlePtr->desiredArsMode_)) {
      continue;
    }

    if (newFlowletConfig) {
      handlePtr->nextHopGroup->setOptionalAttribute(
          SaiNextHopGroupTraits::Attributes::ArsObjectId{arsSaiId});
    } else {
      // flowlet config removal scenario
      handlePtr->nextHopGroup->setOptionalAttribute(
          SaiNextHopGroupTraits::Attributes::ArsObjectId{SAI_NULL_OBJECT_ID});
    }
  }
#endif
}

void SaiNextHopGroupManager::setPrimaryArsSwitchingMode(
    std::optional<cfg::SwitchingMode> switchingMode) {
  primaryArsMode_ = switchingMode;
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

cfg::SwitchingMode SaiNextHopGroupManager::getNextHopGroupSwitchingMode(
    const RouteNextHopEntry::NextHopSet& swNextHops) {
  auto nextHopGroupHandle =
      handles_.get(SaiNextHopGroupKey(swNextHops, std::nullopt));
  if (!nextHopGroupHandle) {
    // if not in dynamic mode, search for backup modes
    std::vector<cfg::SwitchingMode> modes = {
        cfg::SwitchingMode::FIXED_ASSIGNMENT,
        cfg::SwitchingMode::PER_PACKET_RANDOM};
    for (const auto& mode : modes) {
      nextHopGroupHandle = handles_.get(SaiNextHopGroupKey(swNextHops, mode));
      if (nextHopGroupHandle) {
        break;
      }
    }
  }

  if (nextHopGroupHandle && nextHopGroupHandle->desiredArsMode_.has_value()) {
    return nextHopGroupHandle->desiredArsMode_.value();
  }
  return cfg::SwitchingMode::FIXED_ASSIGNMENT;
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
std::pair<
    std::optional<SaiNextHopGroupMemberTraits::AdapterHostKey>,
    std::optional<SaiNextHopGroupMemberTraits::CreateAttributes>>
ManagedSaiNextHopGroupMember<
    NextHopTraits>::getAdapterHostKeyAndCreateAttributes() {
  return std::make_pair(adapterHostKey_, createAttributes_);
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

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0) || defined(BRCM_SAI_SDK_XGS_GTE_13_0)
  if (nhgroup_ && nhgroup_->bulkCreate) {
    adapterHostKey_ = adapterHostKey;
    createAttributes_ = createAttributes;
  } else {
    auto object = manager_->createSaiObject(adapterHostKey, createAttributes);
    this->setObject(object);
  }
#else
  auto object = manager_->createSaiObject(adapterHostKey, createAttributes);
  this->setObject(object);
#endif

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
  this->createAttributes_ = std::nullopt;
  this->adapterHostKey_ = std::nullopt;
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

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0) || defined(BRCM_SAI_SDK_XGS_GTE_13_0)
  if (platform_ &&
      platform_->getAsic()->isSupported(
          HwAsic::Feature::BULK_CREATE_ECMP_MEMBER)) {
    std::vector<SaiNextHopGroupMemberTraits::AdapterKey> adapterKeys;
    for (const auto& member : members_) {
      auto obj = member->getObject();
      if (obj) {
        obj->setSkipRemove(true);
        adapterKeys.emplace_back(obj->adapterKey());
      }
    }

    if (adapterKeys.size()) {
      SaiApiTable::getInstance()->getApi<NextHopGroupApi>().bulkRemove(
          adapterKeys);
    }
  }
#endif

  // Clean up ECMP members in reverse order. Due to the feature of SHEL and ECMP
  // member placement in the hardware, removing ECMP members from head become
  // expensive.
  // In order to maintain a continuous block of ECMP members, SDK will need to
  // move the last member to the removed spot. Rather, remove the memebers in
  // FILO order to optimize the remove performance.
  while (!members_.empty()) {
    members_.pop_back();
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
