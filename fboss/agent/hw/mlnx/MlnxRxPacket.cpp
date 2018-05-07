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
#include "fboss/agent/hw/mlnx/MlnxRxPacket.h"
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"
#include "fboss/agent/hw/mlnx/MlnxPortTable.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/FbossError.h"

#include <folly/MacAddress.h>

#include <netinet/if_ether.h>

using folly::IOBuf;

namespace {
// some additional defines needed here
constexpr auto kVlanTagSize = 4;
constexpr auto kVlanFieldSize = 2;
constexpr auto kEthertypeFieldSize = 2;
constexpr auto kEtherSize = sizeof(ethhdr);
constexpr auto kTaggedEtherSize = kEtherSize + kVlanTagSize;
}

namespace facebook { namespace fboss {

MlnxRxPacket::MlnxRxPacket(const MlnxSwitch* hw, const RxDataStruct& rxData) {
  void* pkt = rxData.pkt;
  uint32_t len = rxData.len;

  if (pkt == nullptr || len <= 0) {
    throw FbossError("Received zero-length packet");
  }

  // Each packet destinated to CPU is expected to be VLAN tagged
  // This method inserts a tag into packet based on ingress vlan id

  len_ = len > kMinPacketSize ? len : kMinPacketSize;
  // source logical port packet came from
  auto log_port = rxData.recv_info.source_log_port;
  auto mlnxPort = hw->getMlnxPortTable()->getMlnxPort(log_port);
  srcPort_ =  mlnxPort->getPortId();

  buf_ = IOBuf::create(len_ + kVlanTagSize);
  buf_->append(len_ + kVlanTagSize);

  // put a cursor pointer at the beginning of the packet
  folly::io::RWPrivateCursor cursor(buf_.get());

  // copy Ethernet header + maybe VLAN tag from pkt (18 bytes)
  cursor.push((uint8_t*)pkt, kTaggedEtherSize);
  cursor.reset(buf_.get());

  // skip srcMac and dstMac
  cursor += folly::MacAddress::SIZE*2;

  // get the etherType of the packet
  auto etherType = cursor.readBE<uint16_t>();


  if (etherType == ETHERTYPE_VLAN) {
    // VLAN tagged packet
    // Just copy the rest of the packet content

    // source VLAN set from VLAN tag
    srcVlan_ = VlanID(cursor.readBE<uint16_t>());
    // skip ethertype
    cursor += kEthertypeFieldSize;

    cursor.push((const uint8_t*)pkt + kTaggedEtherSize,
        len - kTaggedEtherSize);

    buf_->trimEnd(kVlanTagSize);

  } else {
    // No tag included
    // According to FBOSS assumptions we include the VLAN tag
    // VLAN is set to port ingress VLAN
    const auto& swPortTable = hw->getSwSwitch()->getState()->getPorts();
    const auto& swPort = swPortTable->getPort(srcPort_);
    srcVlan_ = swPort->getIngressVlan();

    // need to tag a packet
    // extend by 4 bytes of VLAN tag
    len_ += kVlanTagSize;

    // write VLAN ethertype
    cursor -= kEthertypeFieldSize;
    cursor.writeBE<uint16_t>(ETHERTYPE_VLAN);

    // write VLAN tag
    cursor.writeBE<uint16_t>(srcVlan_);

    // write original ethertype from packet
    cursor.writeBE<uint16_t>(etherType);

    // copy rest of the packet
    cursor.push((const uint8_t*)pkt + kEtherSize,
      len - kEtherSize);
  }
}

}}

