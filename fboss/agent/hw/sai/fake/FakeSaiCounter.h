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

#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <vector>

extern "C" {
#include <sai.h>
}
namespace facebook::fboss {

class FakeCounter {
 public:
  FakeCounter();
  void setType(sai_counter_type_t type) {
    counterType_ = type;
  }

  sai_counter_type_t getType() const {
    return counterType_;
  }

  void setLabel(const sai_attribute_t* attr) {
    std::copy(
        attr->value.chardata,
        attr->value.chardata + label_.size(),
        std::begin(label_));
  }

  void getLabel(sai_attribute_t* attr) const {
    std::copy(std::begin(label_), std::end(label_), attr->value.chardata);
  }

  void setLabelExtended(const sai_attribute_t* attr) {
    labelExtended_.assign(
        attr->value.s8list.list,
        attr->value.s8list.list + attr->value.s8list.count);
  }

  sai_status_t getLabelExtended(sai_attribute_t* attr) const {
    if (attr->value.s8list.count < labelExtended_.size()) {
      attr->value.s8list.count = static_cast<uint32_t>(labelExtended_.size());
      return SAI_STATUS_BUFFER_OVERFLOW;
    }
    attr->value.s8list.count = static_cast<uint32_t>(labelExtended_.size());
    std::copy(
        labelExtended_.begin(), labelExtended_.end(), attr->value.s8list.list);
    return SAI_STATUS_SUCCESS;
  }
  sai_object_id_t id;

 private:
  sai_counter_type_t counterType_;
  SaiCharArray32 label_;
  std::vector<int8_t> labelExtended_;
};

using FakeCounterManager = FakeManager<sai_object_id_t, FakeCounter>;

void populate_counter_api(sai_counter_api_t** counter_api);

} // namespace facebook::fboss
