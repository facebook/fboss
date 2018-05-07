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
#include "fboss/agent/hw/mlnx/MlnxNeighTable.h"
#include "fboss/agent/hw/mlnx/MlnxNeigh.h"
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"

#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"

namespace facebook { namespace fboss {

MlnxNeighTable::MlnxNeighTable(MlnxSwitch* hw) :
  hw_(hw) {
}

MlnxNeighTable::~MlnxNeighTable() {
}

template <typename NeighEntryT>
void MlnxNeighTable::addNeigh(const std::shared_ptr<NeighEntryT>& swNeigh) {
  // construct a key for this neighbor
  InterfaceID ifId = swNeigh->getIntfID();
  const folly::IPAddress& ip = swNeigh->getIP();
  const folly::MacAddress& mac = swNeigh->getMac();
  MlnxNeighKey key {ifId, ip};

  // create and insert MlnxNeigh in table
  neighbors_.emplace(key, std::make_unique<MlnxNeigh>(hw_, key, mac));
}

template <typename NeighEntryT>
void MlnxNeighTable::deleteNeigh(const std::shared_ptr<NeighEntryT>& swNeigh) {
  // construct a key for this neighbor
  InterfaceID ifId = swNeigh->getIntfID();
  const folly::IPAddress& ip = swNeigh->getIP();
  MlnxNeighKey key {ifId, ip};

  // delete MlnxNeigh and erase from table
  neighbors_.erase(key);
}

// force compiler to generate code for NeighEntryT=ArpEntry
// and NeighEntryT=NdpEnty

template void
MlnxNeighTable::addNeigh(const std::shared_ptr<ArpEntry>&);

template void
MlnxNeighTable::addNeigh(const std::shared_ptr<NdpEntry>&);

template void
MlnxNeighTable::deleteNeigh(const std::shared_ptr<ArpEntry>&);

template void
MlnxNeighTable::deleteNeigh(const std::shared_ptr<NdpEntry>&);

}} // facebook::fboss
