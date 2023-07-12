/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"

#include "fboss/agent/hw/bcm/BcmMirrorUtils.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmSdkVer.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <boost/range/combine.hpp>
#include <folly/logging/xlog.h>

extern "C" {
#include <bcm/error.h>

#if (defined(IS_OPENNSA))
// TODO (skhare) Find OpenNSA method for this
#define sal_memset memset
#else
#include <bcm/field.h>
#include <sal/core/libc.h>
#endif
}

using facebook::fboss::bcmCheckError;

DEFINE_int32(
    qset_cmp_gid,
    9999,
    "Temporary group used for comparing "
    "QSETS of already created groups");
DEFINE_int32(
    qset_cmp_pri,
    9999,
    "Temporary group pri for comparing "
    "QSETS of already created groups");

namespace {
int collect_fp_group_ids(int /*unit*/, bcm_field_group_t gid, void* gids) {
  static_cast<std::vector<bcm_field_group_t>*>(gids)->push_back(gid);
  return 0;
}
} // unnamed namespace

namespace facebook::fboss::utility {

std::vector<bcm_field_group_t> fpGroupsConfigured(int unit) {
  std::vector<bcm_field_group_t> gids;
  bcm_field_group_traverse(unit, collect_fp_group_ids, &gids);
  return gids;
}

bool isRedirectToNextHopStateSame(
    const BcmSwitch* hw,
    uint32_t egressId,
    const std::optional<MatchAction>& swAction,
    const std::string& aclMsg,
    const RouterID& routerId) {
  if (!swAction || !swAction.value().getRedirectToNextHop()) {
    XLOG(ERR) << aclMsg
              << " has redirect to nexthops action in h/w but not in s/w";
    return false;
  }
  const auto& resolvedNexthops =
      swAction.value().getRedirectToNextHop().value().second;
  bcm_if_t expectedEgressId;
  if (resolvedNexthops.size() > 0) {
    std::shared_ptr<BcmMultiPathNextHop> multipathNh =
        hw->writableMultiPathNextHopTable()->referenceOrEmplaceNextHop(
            BcmMultiPathNextHopKey(routerId, resolvedNexthops));
    expectedEgressId = multipathNh->getEgressId();
  } else {
    expectedEgressId = hw->getDropEgressId();
  }
  if (egressId != static_cast<uint32_t>(expectedEgressId)) {
    XLOG(ERR) << aclMsg << " redirect nexthop egressId mismatched. SW expected="
              << expectedEgressId << " HW value=" << egressId;
    return false;
  }
  return true;
}

bool isSendToQueueStateSame(
    bcm_field_action_t bcmAction,
    uint32_t bcmQueueId,
    std::optional<MatchAction> swAction,
    const std::string& aclMsg) {
  if (!swAction || !swAction.value().getSendToQueue()) {
    XLOG(ERR) << aclMsg << " has sendToQueue action in h/w but not in s/w";
    return false;
  }
  auto sendToQueue = swAction.value().getSendToQueue().value();
  // check whether queue id matches
  if (bcmQueueId != *sendToQueue.first.queueId()) {
    XLOG(ERR) << aclMsg << " has sendToQueue action, in s/w queue="
              << *sendToQueue.first.queueId() << " in h/w queue=" << bcmQueueId;
    return false;
  }
  // check whether queue type matches
  bool sendToCPU = sendToQueue.second;
  if ((bcmAction == bcmFieldActionCosQNew && sendToCPU) ||
      (bcmAction == bcmFieldActionCosQCpuNew && !sendToCPU)) {
    XLOG(ERR) << aclMsg
              << " has sendToQueue action, in s/w sendToCPU=" << sendToCPU
              << " in h/w sendToCPU is opposite";
    return false;
  }
  return true;
}

bool isSetDscpActionStateSame(
    uint32_t dscpValue,
    std::optional<MatchAction> swAction,
    const std::string& aclMsg) {
  if (!swAction || !swAction.value().getSetDscp()) {
    XLOG(ERR) << aclMsg << " has DscpNewValue action in h/w but not in s/w";
    return false;
  }
  auto setDscp = swAction.value().getSetDscp().value();
  if (dscpValue != *setDscp.dscpValue()) {
    XLOG(ERR) << aclMsg << " has DscpNewValue action, in s/w dscpValue="
              << *setDscp.dscpValue() << " in h/w dscpVaue=" << dscpValue;
    return false;
  }

  return true;
}

bool isMirrorActionSame(
    bcm_field_action_t action,
    uint32_t param0,
    uint32_t param1,
    std::optional<MatchAction> swAction,
    BcmMirrorHandle mirrorHandle,
    const std::string& aclMsg) {
  CHECK(
      action == bcmFieldActionMirrorIngress ||
      action == bcmFieldActionMirrorEgress)
      << " invalid mirror action " << action;

  auto mirror = action == bcmFieldActionMirrorIngress
      ? swAction->getIngressMirror()
      : swAction->getEgressMirror();
  if (!mirror) {
    XLOG(ERR) << aclMsg << " has "
              << mirrorDirectionName(bcmAclMirrorActionToDirection(action))
              << " mirror in h/w, but not in s/w";
    return false;
  }
  return param0 == 0 && param1 == mirrorHandle;
}

bool isActionStateSame(
    const BcmSwitch* hw,
    int unit,
    bcm_field_entry_t entry,
    const std::shared_ptr<AclEntry>& acl,
    const std::string& aclMsg,
    const BcmAclActionParameters& data) {
  // first we need to get all actions of current acl entry
  std::array<bcm_field_action_t, 7> supportedActions = {
      bcmFieldActionDrop,
      bcmFieldActionCosQNew,
      bcmFieldActionCosQCpuNew,
      bcmFieldActionDscpNew,
      bcmFieldActionMirrorIngress,
      bcmFieldActionMirrorEgress,
      bcmFieldActionL3Switch};
  boost::container::flat_map<bcm_field_action_t, std::pair<uint32_t, uint32_t>>
      bcmActions;
  for (auto action : supportedActions) {
    uint32_t param0 = 0, param1 = 0;
    auto rv = bcm_field_action_get(unit, entry, action, &param0, &param1);
    if (rv == BCM_E_NOT_FOUND) {
      continue;
    }
    bcmCheckError(rv, aclMsg, " failed to get action=", action);
    bcmActions.emplace(action, std::make_pair(param0, param1));
  }

  // expcect action count
  int expectedAC = 0;
  expectedAC += acl->getActionType() == cfg::AclActionType::DENY;
  if (acl->getAclAction()) {
    if (acl->getAclAction()->cref<switch_state_tags::sendToQueue>()) {
      expectedAC += 1;
    }
    if (acl->getAclAction()->cref<switch_state_tags::setDscp>()) {
      expectedAC += 1;
    }
    if (acl->getAclAction()->cref<switch_state_tags::ingressMirror>()) {
      expectedAC += 1;
    }
    if (acl->getAclAction()->cref<switch_state_tags::egressMirror>()) {
      expectedAC += 1;
    }
    if (acl->getAclAction()->cref<switch_state_tags::redirectToNextHop>()) {
      expectedAC += 1;
    }
  }
  if (expectedAC != bcmActions.size()) {
    XLOG(ERR) << aclMsg << " has " << expectedAC << "actions in s/w, "
              << "but can only find " << bcmActions.size() << " in h/w";
    return false;
  }
  // if both s/w and h/w have zero action count, we can return true directly
  if (bcmActions.size() == 0) {
    return true;
  }
  bool isSame = true;
  for (auto action = bcmActions.begin(); action != bcmActions.end(); action++) {
    auto param0 = action->second.first;
    auto param1 = action->second.second;
    std::optional<MatchAction> aclAction{};
    if (acl->getAclAction()) {
      // THRIFT_COPY
      aclAction = MatchAction::fromThrift(acl->getAclAction()->toThrift());
    }
    switch (action->first) {
      case bcmFieldActionDrop:
        if (acl->getActionType() != cfg::AclActionType::DENY) {
          XLOG(ERR) << aclMsg << " has drop action in h/w but not in s/w";
          isSame = false;
        }
        break;
      case bcmFieldActionCosQNew:
        isSame = isSendToQueueStateSame(
            bcmFieldActionCosQNew, param0, aclAction, aclMsg);
        break;
      case bcmFieldActionCosQCpuNew:
        isSame = isSendToQueueStateSame(
            bcmFieldActionCosQCpuNew, param0, aclAction, aclMsg);
        break;
      case bcmFieldActionDscpNew:
        isSame = isSetDscpActionStateSame(param0, aclAction, aclMsg);
        break;
      // check for mirror
      case bcmFieldActionMirrorIngress:
        isSame = data.mirrors.ingressMirrorHandle.has_value() &&
            isMirrorActionSame(
                     bcmFieldActionMirrorIngress,
                     param0,
                     param1,
                     aclAction,
                     data.mirrors.ingressMirrorHandle.value(),
                     aclMsg);
        break;
      case bcmFieldActionMirrorEgress:
        isSame = data.mirrors.egressMirrorHandle.has_value() &&
            isMirrorActionSame(
                     bcmFieldActionMirrorEgress,
                     param0,
                     param1,
                     aclAction,
                     data.mirrors.egressMirrorHandle.value(),
                     aclMsg);
        break;
      case bcmFieldActionL3Switch:
        isSame = isRedirectToNextHopStateSame(hw, param0, aclAction, aclMsg);
        break;
      default:
        throw FbossError("Unknown action=", action->first);
    }
  }
  return isSame;
}

void clearFPGroup(int unit, bcm_field_group_t gid) {
  // Blow away entries in the old group and fill in the
  // new addresses. We could be smarter
  int entryCount = 0;
  auto rv = bcm_field_entry_multi_get(unit, gid, 0, nullptr, &entryCount);
  bcmCheckError(rv, "Could not get number of entries in: ", gid);
  if (entryCount) {
    bcm_field_entry_t entries[entryCount];
    rv = bcm_field_entry_multi_get(unit, gid, entryCount, entries, &entryCount);
    bcmCheckError(rv, "Could not get entries in: ", gid);
    for (auto i = 0; i < entryCount; ++i) {
      rv = bcm_field_entry_destroy(unit, entries[i]);
      bcmCheckError(
          rv,
          "Could not remove and destroy entry: ",
          entries[i],
          " in FP group: ",
          gid);
    }
  }
  rv = bcm_field_group_destroy(unit, gid);
  bcmCheckError(rv, "Failed to destroy group: ", gid);
}

void createFPGroup(
    int unit,
    bcm_field_qset_t qset,
    bcm_field_group_t gid,
    int g_pri,
    bool onHSDK) {
  int rv;
  if (onHSDK) {
    bcm_field_group_config_t config;
    bcm_field_group_config_t_init(&config);
    config.flags = BCM_FIELD_GROUP_CREATE_WITH_ID;
    config.qset = qset;
    BCM_FIELD_ASET_INIT(config.aset);
    config.priority = g_pri;
    config.group = gid;
    rv = bcm_field_group_config_create(unit, &config);
  } else {
    rv = bcm_field_group_create_id(unit, qset, g_pri, gid);
  }

  bcmCheckError(rv, "failed to create fp group: ", gid);
  XLOG(DBG1) << " Created FP group: " << gid;
}

bcm_field_qset_t getAclQset(cfg::AsicType asicType) {
  bcm_field_qset_t qset;
  BCM_FIELD_QSET_INIT(qset);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp6);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcPort);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstPort);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpFrag);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDSCP);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTtl);

  /*
   * On Trident2, configuring additional qualifier results in SDK running out of
   * of resources "Failed to create fp group: 128: No resources for operation".
   * bcmFieldQualifyDstClassL2 qualifier is required for MH-NIC queue-per-host
   * solution. However, the solution is not applicable for Trident2 as Trident2
   * does not support queues. Thus, skip configuring this qualifier for Trident2
   */
  if (asicType != cfg::AsicType::ASIC_TYPE_TRIDENT2) {
    BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstClassL2);
    /* Used for counting mpls lookup miss currently. Not used on trident2 */
    BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyPacketRes);
    BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);
    BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlanId);
  }

  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstClassL3);
  return qset;
}

bcm_field_qset_t getGroupQset(int unit, bcm_field_group_t groupId) {
  bcm_field_qset_t qset;
  BCM_FIELD_QSET_INIT(qset);
  auto rv = bcm_field_group_get(unit, groupId, &qset);
  bcmCheckError(rv, "Unable to get qset for group: ", groupId);
  return qset;
}

bool qsetsEqual(const bcm_field_qset_t& lhs, const bcm_field_qset_t& rhs) {
  // Running this loop over groupQset_ and desiredQset_ returns
  // false +ves even though qsets are equal
  for (auto qual = 0; qual < bcmFieldQualifyCount; ++qual) {
    if ((BCM_FIELD_QSET_TEST(lhs, qual) && !BCM_FIELD_QSET_TEST(rhs, qual)) ||
        (BCM_FIELD_QSET_TEST(rhs, qual) && !BCM_FIELD_QSET_TEST(lhs, qual))) {
      return false;
    }
  }
  return true;
}

bool fpGroupExists(int unit, bcm_field_group_t gid) {
  bcm_field_qset_t qset;
  BCM_FIELD_QSET_INIT(qset);
  auto rv = bcm_field_group_get(unit, gid, &qset);
  // Be strict in checking error code on group get,
  // so as to not falsely conclude a group is missing
  // in presence of other errors to fetch the group.
  return rv != BCM_E_NOT_FOUND;
}

int fpGroupNumAclEntries(int unit, bcm_field_group_t gid) {
  int size;
  auto rv = bcm_field_entry_multi_get(unit, gid, 0, nullptr, &size);
  bcmCheckError(
      rv,
      "failed to get field group entry count, gid=",
      folly::to<std::string>(gid));
  return size;
}

bool needsExtraFPQsetQualifiers(cfg::AsicType asicType) {
  // Currently we know any asic is before TH4 will add extra qualifiers
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
      return true;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      return false;
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      throw FbossError("Unsupported ASIC type");
  }
  return true;
}

FPGroupDesiredQsetCmp::FPGroupDesiredQsetCmp(
    const BcmSwitch* hw,
    bcm_field_group_t groupId,
    const bcm_field_qset_t& desiredQset)
    : unit_(hw->getUnit()),
      needsExtraFPQsetQualifiers_(needsExtraFPQsetQualifiers(
          hw->getPlatform()->getAsic()->getAsicType())),
      groupId_(groupId),
      groupQset_(getGroupQset(unit_, groupId)),
      desiredQset_(desiredQset) {}

bool FPGroupDesiredQsetCmp::hasDesiredQset() {
  auto equal = qsetsEqual(groupQset_, getEffectiveDesiredQset());
  XLOG(DBG2) << "Group : " << groupId_ << " has desired qset: " << equal;
  return equal;
}

bcm_field_qset_t FPGroupDesiredQsetCmp::getEffectiveDesiredQset() {
  // Unfortunately just comparing desiredQset with the qset
  // of a configured group, does not do the right thing. Even
  // if the group was configured with the exact same qset. In
  // particular a qset obtained from a configured FP group
  // always has bcmFieldQualifyStage qualifier set.
  // post group configure
  // I haven't gotten a great answer from BRCM on this besides
  // the fact that they use this to identify FP stage.
  // We used to create a temporary group with desiredQset and
  // then compare it with groupQset. But creating any group post
  // warmboot hoses some of the existing matches - T26329911. So
  // unfortunately we have to leak this BRCM internal detail
  // into our code. Tests have been added to ensure that if this
  // ever changes on a new SDK.
  // Bug is fixed in 6.5.12+ so we could go back to creating temp
  // group and not maintain this detail in our code.
  if (needsExtraFPQsetQualifiers_) {
    bcm_field_qset_t effectiveDesiredQset = desiredQset_;
    BCM_FIELD_QSET_ADD(effectiveDesiredQset, bcmFieldQualifyStage);
    return effectiveDesiredQset;
  } else {
    return desiredQset_;
  }
}

} // namespace facebook::fboss::utility
