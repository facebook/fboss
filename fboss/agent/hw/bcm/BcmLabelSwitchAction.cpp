// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabelSwitchAction.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmLabelSwitchingUtils.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmTypes.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

namespace {
bool areEquivalent(
    const bcm_mpls_tunnel_switch_t& lhs,
    const bcm_mpls_tunnel_switch_t& rhs) {
  return lhs.label == rhs.label && lhs.port == rhs.port &&
      lhs.action == rhs.action && lhs.egress_if == rhs.egress_if &&
      ((lhs.flags & BCM_MPLS_SWITCH_TTL_DECREMENT) ==
       (rhs.flags & BCM_MPLS_SWITCH_TTL_DECREMENT));
}
} // namespace
namespace facebook::fboss {

BcmLabelSwitchAction::BcmLabelSwitchAction(
    BcmSwitch* hw,
    bcm_mpls_label_t topLabel,
    const LabelNextHopEntry& entry)
    : unit_(hw->getUnit()) {
  // using vrf 0 by default for host references, if going forward more than one
  // vrf gets supported, next hop address or label forwarding info  entry base
  // must carry which vrf to use to resolve next hop l3 address

  bcm_mpls_tunnel_switch_t_init(&action_);
  action_.label = topLabel;
  action_.flags = BCM_MPLS_SWITCH_TTL_DECREMENT; // decrement TTL by 1
  action_.port = BCM_GPORT_INVALID; // platform label space
  action_.action = utility::getLabelSwitchAction(
      entry.getAction(),
      entry.getNextHopSet().begin()->labelForwardingAction()->type());

  auto defaultDataPlaneQosPolicy =
      hw->getQosPolicyTable()->getDefaultDataPlaneQosPolicyIf();
  if (defaultDataPlaneQosPolicy) {
    action_.flags |= BCM_MPLS_SWITCH_INT_PRI_MAP;
    action_.qos_map_id =
        defaultDataPlaneQosPolicy->getHandle(BcmQosMap::Type::MPLS_INGRESS);
  }
  auto labelForwardingType =
      entry.getNextHopSet().begin()->labelForwardingAction()->type();
  if (labelForwardingType !=
      LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP) {
    /* pop and look up does not require any egress if settings */
    if (labelForwardingType ==
        LabelForwardingAction::LabelForwardingType::PHP) {
      // put decremented TTL into outgoing L3 packets
      action_.flags |= BCM_MPLS_SWITCH_OUTER_TTL;
      /* PHP reuses same egress objects as those of L3, because PHP are
       * unlabeled next hops */
      nexthop_ = hw->writableMultiPathNextHopTable()->referenceOrEmplaceNextHop(
          BcmMultiPathNextHopKey(
              0 /* vrfid */,
              utility::stripLabelForwarding(entry.normalizedNextHops())));
    } else {
      // put decremented TTL into outgoing L3 packets
      action_.flags |= BCM_MPLS_SWITCH_OUTER_TTL;
      nexthop_ = hw->writableMultiPathNextHopTable()->referenceOrEmplaceNextHop(
          BcmMultiPathNextHopKey(0 /* vrfid */, entry.normalizedNextHops()));
    }
    action_.egress_if = nexthop_->getEgressId();
  }

  auto itr = hw->getWarmBootCache()->findLabelAction(action_.label);
  bool exists = false;
  if (itr != hw->getWarmBootCache()->Label2LabelActionMapEnd()) {
    if (areEquivalent(itr->second->get()->data, action_)) {
      XLOG(DBG3) << "label switch action for label " << topLabel
                 << " already exists";
      exists = true;
    }
    XLOG(DBG3) << "replacing label switch action for label " << topLabel;
    action_.flags |= BCM_MPLS_SWITCH_REPLACE;
  } else {
    XLOG(DBG3) << "adding label switch action for label " << topLabel;
  }
  if (!exists) {
    bcmCheckError(
        bcm_mpls_tunnel_switch_add(unit_, &action_),
        "failed to program label ",
        topLabel);
  }
  if (itr != hw->getWarmBootCache()->Label2LabelActionMapEnd()) {
    hw->getWarmBootCache()->programmedLabelAction(itr);
  }
}

BcmLabelSwitchAction::~BcmLabelSwitchAction() {
  bcmCheckError(
      bcm_mpls_tunnel_switch_delete(unit_, &action_),
      "failed to remove label ",
      action_.label);
}

} // namespace facebook::fboss
