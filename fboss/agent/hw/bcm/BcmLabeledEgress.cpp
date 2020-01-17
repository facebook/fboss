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
  auto bcmEgress = reinterpret_cast<bcm_l3_egress_t*>(eObj);
  bcmEgress->mpls_label = label_;
  auto defaultDataPlaneQosPolicy =
      hw_->getQosPolicyTable()->getDefaultDataPlaneQosPolicyIf();
  if (defaultDataPlaneQosPolicy) {
    bcmEgress->mpls_qos_map_id =
        defaultDataPlaneQosPolicy->getHandle(BcmQosMap::Type::MPLS_EGRESS);
    bcmEgress->mpls_flags |= BCM_MPLS_EGRESS_LABEL_EXP_REMARK;
  }
}

} // namespace facebook::fboss
