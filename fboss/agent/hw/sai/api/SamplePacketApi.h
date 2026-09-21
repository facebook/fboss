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
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>
#include <variant>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class SamplePacketApi;

struct SaiSamplePacketTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_SAMPLEPACKET;
  using SaiApiT = SamplePacketApi;
  struct Attributes {
    using EnumType = sai_samplepacket_attr_t;
    using SampleRate =
        SaiAttribute<EnumType, SAI_SAMPLEPACKET_ATTR_SAMPLE_RATE, sai_uint32_t>;
    using Type =
        SaiAttribute<EnumType, SAI_SAMPLEPACKET_ATTR_TYPE, sai_int32_t>;
    using Mode =
        SaiAttribute<EnumType, SAI_SAMPLEPACKET_ATTR_MODE, sai_int32_t>;
  };
  using AdapterKey = SamplePacketSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::SampleRate, Attributes::Type, Attributes::Mode>;
  using CreateAttributes = AdapterHostKey;
};

SAI_ATTRIBUTE_NAME(SamplePacket, SampleRate)
SAI_ATTRIBUTE_NAME(SamplePacket, Type)
SAI_ATTRIBUTE_NAME(SamplePacket, Mode)

class SamplePacketApi : public SaiApi<SamplePacketApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_SAMPLEPACKET;
  SamplePacketApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for sample packet api");
  }

 private:
  sai_status_t _create(
      SamplePacketSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_samplepacket(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(SamplePacketSaiId id) const {
    return api_->remove_samplepacket(id);
  }
  sai_status_t _getAttribute(SamplePacketSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_samplepacket_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(SamplePacketSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_samplepacket_attribute(id, attr);
  }

  sai_samplepacket_api_t* api_;
  friend class SaiApi<SamplePacketApi>;
};

} // namespace facebook::fboss
