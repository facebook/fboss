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
#include "SaiPortTable.h"
#include "SaiPortBase.h"
#include "SaiSwitch.h"
#include "SaiPlatformBase.h"
#include "SaiPlatformPort.h"
#include "SaiError.h"

using folly::make_unique;
using std::unique_ptr;
using std::make_pair;

namespace facebook { namespace fboss {

SaiPortTable::SaiPortTable(SaiSwitch *pSwitch)
  :pHw_(pSwitch) {
  VLOG(4) << "Entering " << __FUNCTION__;
}

SaiPortTable::~SaiPortTable() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

void SaiPortTable::InitPorts(bool warmBoot) {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto platformPorts = pHw_->GetPlatform()->initPorts();

  for (const auto& entry : platformPorts) {
      sai_object_id_t saiPortId = entry.first;
      SaiPlatformPort *platformPort = entry.second;

      PortID fbossPortID = platformPort->getPortID();
      auto saiPortBase = make_unique<SaiPortBase>(pHw_, saiPortId, fbossPortID, platformPort);
      platformPort->SetPort(saiPortBase.get());
      saiPortBase->Init(warmBoot);

      fbossPhysicalPorts_.emplace(fbossPortID, saiPortBase.get());
      saiPhysicalPorts_.emplace(saiPortId, std::move(saiPortBase));
  }
}

sai_object_id_t SaiPortTable::GetSaiPortId(PortID id) const {
    return GetSaiPort(id)->GetSaiPortId();
}

PortID SaiPortTable::GetPortId(sai_object_id_t portId) const {
    return GetSaiPort(portId)->GetFbossPortId();
}

SaiPortBase *SaiPortTable::GetSaiPort(PortID id) const {
  VLOG(6) << "Entering " << __FUNCTION__;

  auto iter = fbossPhysicalPorts_.find(id);

  if (iter == fbossPhysicalPorts_.end()) {
    throw SaiError("Cannot find the SAI port object for FBOSS port ID ", id);
  }

  return iter->second;
}

SaiPortBase *SaiPortTable::GetSaiPort(sai_object_id_t id) const {
  VLOG(6) << "Entering " << __FUNCTION__;

  auto iter = saiPhysicalPorts_.find(id);

  if (iter == saiPhysicalPorts_.end()) {
    std::stringstream stream;
    stream << "0x" << std::hex << id;
    throw SaiError("Cannot find the SAI port object for SAI port ", stream.str());
  }

  return iter->second.get();
}

void SaiPortTable::SetPortStatus(sai_object_id_t portId, int status) {
  VLOG(6) << "Entering " << __FUNCTION__;

  auto port = GetSaiPort(portId);
  port->SetPortStatus(status);
}

void SaiPortTable::UpdatePortStats() {
  VLOG(6) << "Entering " << __FUNCTION__;

  for (const auto& entry : saiPhysicalPorts_) {
    SaiPortBase *saiPort = entry.second.get();
    saiPort->UpdateStats();
  }
}

}} // facebook::fboss
