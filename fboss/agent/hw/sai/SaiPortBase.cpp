/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include "SaiPortBase.h"
#include "SaiSwitch.h"
#include "SaiPlatformPort.h"
#include "SaiError.h"

extern "C" {
#include "sai.h"
}

namespace facebook { namespace fboss {

SaiPortBase::SaiPortBase(SaiSwitch *pSwitch, sai_object_id_t saiPortId, PortID fbossPortId, SaiPlatformPort *pPlatformPort)
  : pHw_(pSwitch),
    pPlatformPort_(pPlatformPort),
    saiPortId_(saiPortId),
    fbossPortId_(fbossPortId) {
  VLOG(6) << "Entering " << __FUNCTION__;

  sai_api_query(SAI_API_PORT, (void **) &pSaiPortApi_);
}

SaiPortBase::~SaiPortBase() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

void SaiPortBase::Init(bool warmBoot) {
  VLOG(6) << "Entering " << __FUNCTION__;

  try {
    SetIngressVlan(pvId_);
  } catch (const SaiError &e) {
    LOG(ERROR) << e.what();
  }

  SetPortStatus(true);

  initDone_ = true;
}

void SaiPortBase::SetPortStatus(bool linkStatus) {
  VLOG(6) << "Entering " << __FUNCTION__;

  if (initDone_ && (linkStatus_ == linkStatus)) {
    return;
  }

  sai_attribute_t attr {0};
  attr.id = SAI_PORT_ATTR_ADMIN_STATE;
  attr.value.booldata = linkStatus;
  
  sai_status_t saiStatus = pSaiPortApi_->set_port_attribute(saiPortId_, &attr);
  if(SAI_STATUS_SUCCESS != saiStatus) {
    LOG(ERROR) << "failed to set port status: " << linkStatus << "error: " << saiStatus;
  }

  VLOG(6) << "Set port: " << fbossPortId_.t << " status to: " << linkStatus;
  // We ignore the return value.  If we fail to get the port status
  // we just tell the platformPort_ that it is enabled.
  pPlatformPort_->linkStatusChanged(linkStatus, true);
}

void SaiPortBase::SetIngressVlan(VlanID vlan) {
  VLOG(6) << "Entering " << __FUNCTION__;

  if (initDone_ && (pvId_ == vlan)) {
    return;
  }

  sai_status_t saiStatus = SAI_STATUS_SUCCESS;
  sai_attribute_t attr;

  attr.id = SAI_PORT_ATTR_PORT_VLAN_ID;
  attr.value.u16 = vlan.t;

  saiStatus = pSaiPortApi_->set_port_attribute(saiPortId_, &attr);
  if(SAI_STATUS_SUCCESS != saiStatus) {
    throw SaiError("Failed to update ingress VLAN for port ", fbossPortId_.t);
  }

  VLOG(6) << "Set port: " << fbossPortId_.t << " ingress VLAN to: " << vlan.t;

  pvId_ = vlan;
}

std::string SaiPortBase::StatName(folly::StringPiece name) const {
  VLOG(6) << "Entering " << __FUNCTION__;

  return folly::to<std::string>("port", pPlatformPort_->getPortID(), ".", name);
}

void SaiPortBase::UpdateStats() {
  VLOG(6) << "Entering " << __FUNCTION__;
}

}} // facebook::fboss
