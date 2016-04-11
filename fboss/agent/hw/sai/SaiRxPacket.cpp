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
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

#include "SaiRxPacket.h"
#include "SaiSwitch.h"
#include "SaiPortTable.h"
#include "SaiError.h"
#include "SaiPortBase.h"

extern "C" {
#include "saihostintf.h"
}

using folly::IOBuf;

namespace facebook { namespace fboss {

SaiRxPacket::SaiRxPacket(const void* buf,
                         sai_size_t buf_size,
                         uint32_t attr_count,
                         const sai_attribute_t *attr_list,
                         const SaiSwitch *hw) {

  if (!buf || (buf_size == 0) || (attr_count == 0) ||
      !attr_list || !hw) {

    throw SaiError("Invalid input data.");
  }

  const uint8_t PKT_MIN_LEN = 64;

  for (uint32_t i = 0; i < attr_count; i++) {

    if (attr_list[i].id == SAI_HOSTIF_PACKET_INGRESS_PORT) {
      srcPort_ = hw->GetPortTable()->GetPortId(attr_list[i].value.oid);
    }
  }
  
  // Append ingress packet to 64 bytes.
  len_ = (buf_size < PKT_MIN_LEN) ? PKT_MIN_LEN : buf_size;

  uint8_t *pkt_buf = new uint8_t[len_];
  memset(pkt_buf, 0, len_);
  memcpy(pkt_buf, buf, buf_size);

  buf_ = IOBuf::takeOwnership(pkt_buf,           // void* buf
                              len_,              // uint32_t capacity
                              freeRxBufCallback, // Free Function freeFn
                              NULL);             // void* userData

  folly::io::Cursor c(buf_.get());

  c += folly::MacAddress::SIZE * 2; // skip dst and src MACs

  auto ethertype = c.readBE<uint16_t>();
  if (ethertype == 0x8100) {
    // Packet is tagged with a VLAN so take the VLAN from there.
    srcVlan_ = VlanID(c.readBE<uint16_t>());
  } else {
    // In case of untagged packet we just take the ingress VLAN of the source port.
    srcVlan_ = hw->GetPortTable()->GetSaiPort(srcPort_)->GetIngressVlan();                                      
  }
}

SaiRxPacket::~SaiRxPacket() {
  // Nothing to do.  The IOBuf destructor will call freeRxBuf()
  // to free the packet data
}

void SaiRxPacket::freeRxBufCallback(void *ptr, void* arg) {
  delete[] static_cast<uint8_t*>(ptr);
}

}} // facebook::fboss
