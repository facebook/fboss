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
#pragma once

#include "fboss/agent/types.h"

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_router.h>
}

namespace facebook { namespace fboss {

class MlnxSwitch;

class MlnxVrf {
 public:
  /**
   * ctor, creates VRF in SDK
   *
   * @param hw Pointer to MlnxSwitch
   * @param id FBOSS Router ID
   */
  MlnxVrf(MlnxSwitch* hw, RouterID id);
  /**
   * Returns SDK virtual router id
   *
   * @param Virtual router id
   */
  sx_router_id_t getSdkVrid() const;
  /**
   * dtor, deletes VRF from SDK
   */
  ~MlnxVrf();
 private:
  // forbidden copy ctor and assignment operator
  MlnxVrf(const MlnxVrf&) = delete;
  MlnxVrf& operator=(const MlnxVrf&) = delete;

  // private fields
  MlnxSwitch* hw_ {nullptr};
  sx_api_handle_t handle_ {SX_API_INVALID_HANDLE};
  RouterID id_;
  sx_router_id_t vrid_;
};

}} // facebook::fboss
