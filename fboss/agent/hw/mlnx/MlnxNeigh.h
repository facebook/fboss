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

#include "fboss/agent/state/NeighborEntry.h"

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_router.h>
}

#include <folly/IPAddress.h>

#include <string>

namespace facebook { namespace fboss {

class MlnxSwitch;

/**
 * Structure to have a key for neighbor
 * Contains from Interface ID and IP address of neighbor
 */
struct MlnxNeighKey{
  MlnxNeighKey(InterfaceID ifId,
    const folly::IPAddress ip);
  /**
   * Comparison operator for neighbor key.
   * Required by flat_map implementation
   *
   * @param key Another MlnxNeighKey instance
   * @ret true if this key < another key, otherwise false
   */
  bool operator < (const MlnxNeighKey& key) const;

  InterfaceID ifId_;
  folly::IPAddress ip_;
};

/**
 * Manages a neighbor entry in SDK
 */
class MlnxNeigh {
 public:
  /**
   * ctor, creates neighbor entry in SDK
   * Lifetime of that entry is bound to the lifetime of this object
   * The object is creating only when we have valid MAC (the neighbor is resolved)
   * The action is set to forward
   *
   * @param hw Pointer to MlnxSwitch
   * @param key Neighbor key structure
   * @param mac Mac address for an entry
   */
  MlnxNeigh(MlnxSwitch* hw,
    const MlnxNeighKey& key,
    const folly::MacAddress& mac);

  /**
   * dtor, deletes an entry from SDK
   */
  ~MlnxNeigh();

  /**
   * Serialize neighbor object into string that represents
   * neighbor fields as written to the HW, useful for logs and debug
   *
   * @ret string with neighbor info
   */
  std::string toString() const;

 private:
  // forbidden copy constructor and assignment operator
  MlnxNeigh(const MlnxNeigh&) = delete;
  MlnxNeigh& operator=(const MlnxNeigh&) = delete;

  // private fields

  MlnxSwitch* hw_;
  sx_api_handle_t handle_;
  // SDK values for a neighbor
  sx_router_interface_t rif_;
  sx_ip_addr_t ipAddr_;
  sx_neigh_data_t neighData_;
};

}} // facebook::fboss
