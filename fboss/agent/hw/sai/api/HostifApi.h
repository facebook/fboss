/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"

#include <folly/logging/xlog.h>
#include <folly/MacAddress.h>

#include <vector>

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

struct HostifApiParameters {
  struct TxPacketAttributes {
    using EnumType = sai_hostif_packet_attr_t;
    using TxType = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_HOSTIF_TX_TYPE,
        sai_int32_t>;
    using EgressPortOrLag = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG,
        SaiObjectIdT>;
    using TxAttributes = SaiAttributeTuple
      <TxType,
      SaiAttributeOptional<EgressPortOrLag>>;
    TxPacketAttributes(const TxAttributes& attrs) {
      std::tie(txType, egressPortOrLag) = attrs.value();
    }
    TxAttributes attrs() const {
      return {txType, egressPortOrLag};
    }
    folly::Optional<typename EgressPortOrLag::ValueType> egressPortOrLag;
    typename TxType::ValueType txType;
  };

  struct RxPacketAttributes {
    using EnumType = sai_hostif_packet_attr_t;
    using TrapId = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_HOSTIF_TRAP_ID,
        SaiObjectIdT>;
    using IngressPort = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_INGRESS_PORT,
        SaiObjectIdT>;
    using IngressLag = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_INGRESS_LAG,
        SaiObjectIdT>;
    using RxAttributes = SaiAttributeTuple<TrapId, IngressPort, IngressLag>;
    RxPacketAttributes(const RxAttributes& attrs) {
      std::tie(trapId, ingressPort, ingressLag) = attrs.value();
    }
    RxAttributes attrs() const {
      return {trapId, ingressPort, ingressLag};
    }
    typename TrapId::ValueType trapId;
    typename IngressPort::ValueType ingressPort;
    typename IngressLag::ValueType ingressLag;
  };

  struct HostifApiPacket {
    void* buffer;
    size_t size;
    HostifApiPacket(void* buffer, size_t size) : buffer(buffer), size(size) {}
  };

  struct MemberAttributes {};
  using MemberAttributeType = boost::variant<boost::blank>;
  struct EntryType {};
};

class HostifPacketApi {
public:
  HostifPacketApi() {
    sai_status_t status =
        sai_api_query(SAI_API_HOSTIF, reinterpret_cast<void**>(&api_));
    saiCheckError(status, "Failed to query for switch api");
  }

  sai_status_t send(
    const HostifApiParameters::TxPacketAttributes::TxAttributes &attributes,
    sai_object_id_t switch_id,
    HostifApiParameters::HostifApiPacket &txPacket) {
    std::vector<sai_attribute_t> saiAttributeTs = attributes.saiAttrs();
    return api_->send_hostif_packet(
        switch_id,
        txPacket.size,
        txPacket.buffer,
        saiAttributeTs.size(),
        saiAttributeTs.data());
  }

private:
  sai_hostif_api_t* api_;
  friend class SaiApi<HostifPacketApi, HostifApiParameters>;
};

} // namespace fboss
} // namespace facebook
