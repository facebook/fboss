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
#include "fboss/agent/hw/mlnx/MlnxVrf.h"
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"

extern "C" {
#include <sx/sdk/sx_api_router.h>
}

namespace facebook { namespace fboss {

MlnxVrf::MlnxVrf(MlnxSwitch* hw, RouterID id) :
  hw_(hw),
  handle_(hw_->getHandle()),
  id_(id) {
  sx_status_t rc;
  sx_router_attributes_t routerAttrs;
  std::memset(&routerAttrs, 0, sizeof(routerAttrs));

  // enbale ipv4/ipv6 unicast
  routerAttrs.ipv4_enable = SX_ROUTER_ENABLE_STATE_ENABLE;
  routerAttrs.ipv6_enable = SX_ROUTER_ENABLE_STATE_ENABLE;
  routerAttrs.ipv4_mc_enable = SX_ROUTER_ENABLE_STATE_DISABLE;
  routerAttrs.ipv6_mc_enable = SX_ROUTER_ENABLE_STATE_DISABLE;
  routerAttrs.uc_default_rule_action = SX_ROUTER_ACTION_DROP;
  routerAttrs.mc_default_rule_action = SX_ROUTER_ACTION_DROP;

  // create virtual router
  rc = sx_api_router_set(handle_, SX_ACCESS_CMD_ADD, &routerAttrs, &vrid_);
  mlnxCheckSdkFail(rc,
    "sx_api_router_set",
    "Failed to create VRF in SDK");

  LOG(INFO) << "Virtaul router with @id " << id_
            << " @vrid " << vrid_ << " created";
}

sx_router_id_t MlnxVrf::getSdkVrid() const {
  return vrid_;
}

MlnxVrf::~MlnxVrf() {
  sx_status_t rc;

  // delete virtual router
  rc = sx_api_router_set(handle_, SX_ACCESS_CMD_DELETE, nullptr, &vrid_);
  mlnxLogSxError(rc,
    "sx_api_router_set",
    "Failed to delete a virtual router with @id ",
    id_);

  LOG_IF(INFO, rc == SX_STATUS_SUCCESS)
    << "Virtaul router with @id " << id_
    << " sdk @vrid " << vrid_ << " deleted";
}

}} // facebook::fboss
