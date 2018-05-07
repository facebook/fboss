/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "fboss/agent/hw/mlnx/MlnxIntfTable.h"
#include "fboss/agent/hw/mlnx/MlnxIntf.h"

#include "fboss/agent/FbossError.h"

#include <folly/IPAddress.h>

namespace facebook { namespace fboss {

MlnxIntfTable::MlnxIntfTable (MlnxSwitch* hw) :
  hw_(hw) {
}

MlnxIntfTable::~MlnxIntfTable () {
}

void MlnxIntfTable::addIntf(const std::shared_ptr<Interface> swIntf) {
  auto intf = std::make_unique<MlnxIntf>(hw_, this, *swIntf);
  rifToId_.emplace(intf->getSdkRif(), swIntf->getID());
  intfs_.emplace(swIntf->getID(), std::move(intf));
}

void MlnxIntfTable::changeIntf(const std::shared_ptr<Interface> oldIntf,
  const std::shared_ptr<Interface> newIntf) {
  auto intfIt = intfs_.find(oldIntf->getID());
  if (intfIt == intfs_.end()) {
    return;
  }
  intfIt->second->change(*newIntf);
}

void MlnxIntfTable::deleteIntf(const std::shared_ptr<Interface> swIntf) {
  auto id = swIntf->getID();
  rifToId_.erase(getMlnxIntfById(id)->getSdkRif());
  intfs_.erase(swIntf->getID());
}

MlnxIntfTable::InterfaceMapT::const_iterator MlnxIntfTable::begin() const {
  return intfs_.begin();
}

MlnxIntfTable::InterfaceMapT::const_iterator MlnxIntfTable::end() const {
  return intfs_.end();
}

InterfaceID MlnxIntfTable::getInterfaceIdByRif(
  sx_router_interface_t rif) const {
  auto iter = rifToId_.find(rif);
  if (iter == rifToId_.end()) {
    throw FbossError("No interface with @rif", static_cast<int>(rif));
  }
  return iter->second;
}

MlnxIntfTable::IpToInterfaceMapT* MlnxIntfTable::writableIpToInterfaceMap() {
  return &ipToIntf_;
}

MlnxIntf* MlnxIntfTable::getMlnxIntfByAttachedIpIf(
  const folly::IPAddress& intfIp) const {
  const auto& intfIt = ipToIntf_.find(intfIp);
  if (intfIt == ipToIntf_.end()) {
    return nullptr;
  }
  return intfIt->second;
}

const MlnxIntf* MlnxIntfTable::getMlnxIntfById(InterfaceID id) const {
  auto intfIt = intfs_.find(id);
  if (intfIt == intfs_.end()) {
    return nullptr;
  }
  return intfIt->second.get();
}

}} // facebook::fboss
