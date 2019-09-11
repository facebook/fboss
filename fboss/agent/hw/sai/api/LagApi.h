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

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

class LagApi : public SaiApi<LagApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_LAG;
  LagApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for lag api");
  }
  LagApi(const LagApi& other) = delete;

 private:
  sai_status_t _create(
      LagSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_lag(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _create(
      LagMemberSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_lag_member(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(LagSaiId id) {
    return api_->remove_lag(id);
  }
  sai_status_t _remove(LagMemberSaiId id) {
    return api_->remove_lag_member(id);
  }

  sai_status_t _getAttribute(LagSaiId id, sai_attribute_t* attr) const {
    return api_->get_lag_attribute(id, 1, attr);
  }
  sai_status_t _getAttribute(LagMemberSaiId id, sai_attribute_t* attr) const {
    return api_->get_lag_member_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(LagSaiId id, const sai_attribute_t* attr) {
    return api_->set_lag_attribute(id, attr);
  }
  sai_status_t _setAttribute(LagMemberSaiId id, const sai_attribute_t* attr) {
    return api_->set_lag_member_attribute(id, attr);
  }
  sai_lag_api_t* api_;
  friend class SaiApi<LagApi>;
};

} // namespace fboss
} // namespace facebook
