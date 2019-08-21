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

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

struct QueueApiParameters {
  static constexpr sai_api_t ApiType = SAI_API_QUEUE;
  struct Attributes {
    using EnumType = sai_queue_attr_t;
    using Type = SaiAttribute<EnumType, SAI_QUEUE_ATTR_TYPE, sai_int32_t>;
    using Port = SaiAttribute<EnumType, SAI_QUEUE_ATTR_PORT, SaiObjectIdT>;
    using Index = SaiAttribute<EnumType, SAI_QUEUE_ATTR_INDEX, sai_uint8_t>;
    using ParentSchedulerNode = SaiAttribute<
        EnumType,
        SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE,
        SaiObjectIdT>;
    using WredProfileId =
        SaiAttribute<EnumType, SAI_QUEUE_ATTR_WRED_PROFILE_ID, SaiObjectIdT>;
    using BufferProfileId =
        SaiAttribute<EnumType, SAI_QUEUE_ATTR_BUFFER_PROFILE_ID, SaiObjectIdT>;
    using SchedulerProfileId = SaiAttribute<
        EnumType,
        SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID,
        SaiObjectIdT>;

    using CreateAttributes =
        SaiAttributeTuple<Type, Port, Index, ParentSchedulerNode>;

    Attributes(const CreateAttributes& attrs) {
      std::tie(type, port, index, parentSchedulerNode) = attrs.value();
    }

    CreateAttributes attrs() const {
      return {type, port, index, parentSchedulerNode};
    }
    bool operator==(const Attributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const Attributes& other) const {
      return !(*this == other);
    }
    typename Type::ValueType type;
    typename Port::ValueType port;
    typename Index::ValueType index;
    typename ParentSchedulerNode::ValueType parentSchedulerNode;
  };
};

class QueueApi : public SaiApi<QueueApi, QueueApiParameters> {
 public:
  QueueApi() {
    sai_status_t status =
        sai_api_query(SAI_API_QUEUE, reinterpret_cast<void**>(&api_));
    saiApiCheckError(
        status, QueueApiParameters::ApiType, "Failed to query for queue api");
  }

 private:
  sai_status_t _create(
      sai_object_id_t* queue_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_queue(queue_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t queue_id) {
    return api_->remove_queue(queue_id);
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t queue_id) const {
    return api_->get_queue_attribute(queue_id, 1, attr);
  }
  sai_status_t _setAttr(const sai_attribute_t* attr, sai_object_id_t queue_id) {
    return api_->set_queue_attribute(queue_id, attr);
  }
  sai_queue_api_t* api_;
  friend class SaiApi<QueueApi, QueueApiParameters>;
};

} // namespace fboss
} // namespace facebook
