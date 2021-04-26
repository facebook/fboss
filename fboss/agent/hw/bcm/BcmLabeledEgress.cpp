// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/Interface.h"

extern "C" {
#include <bcm/l3.h>
#include <bcm/mpls.h>
}

namespace {
void setupLabeledEgressProperties(
    bcm_l3_egress_t* bcmEgress,
    bcm_mpls_label_t label,
    facebook::fboss::BcmQosPolicy* qosPolicy) {
  bcmEgress->mpls_label = label;
  if (qosPolicy) {
    bcmEgress->mpls_qos_map_id =
        qosPolicy->getHandle(facebook::fboss::BcmQosMap::Type::MPLS_EGRESS);
    bcmEgress->mpls_flags |= BCM_MPLS_EGRESS_LABEL_EXP_REMARK;
  }
}
} // namespace

namespace facebook::fboss {

BcmWarmBootCache::EgressId2EgressCitr BcmLabeledEgress::findEgress(
    bcm_vrf_t vrf,
    bcm_if_t intfId,
    const folly::IPAddress& ip) const {
  auto* bcmIntf = hw_->getIntfTable()->getBcmIntf(intfId);

  return hw_->getWarmBootCache()->findEgressFromLabeledHostKey(
      BcmLabeledHostKey(vrf, getLabel(), ip, bcmIntf->getInterface()->getID()));
}

void BcmLabeledEgress::prepareEgressObject(
    bcm_if_t intfId,
    bcm_port_t port,
    const std::optional<folly::MacAddress>& mac,
    RouteForwardAction action,
    bcm_l3_egress_t* eObj) const {
  BcmEgress::prepareEgressObject(intfId, port, mac, action, eObj);
  auto defaultDataPlaneQosPolicy =
      hw_->getQosPolicyTable()->getDefaultDataPlaneQosPolicyIf();
  setupLabeledEgressProperties(eObj, label_, defaultDataPlaneQosPolicy);
}

void BcmLabeledEgress::prepareEgressObjectOnTrunk(
    bcm_if_t intfId,
    bcm_trunk_t trunk,
    const folly::MacAddress& mac,
    bcm_l3_egress_t* egress) const {
  BcmEgress::prepareEgressObjectOnTrunk(intfId, trunk, mac, egress);

  auto defaultDataPlaneQosPolicy =
      hw_->getQosPolicyTable()->getDefaultDataPlaneQosPolicyIf();
  setupLabeledEgressProperties(egress, label_, defaultDataPlaneQosPolicy);
}

} // namespace facebook::fboss
