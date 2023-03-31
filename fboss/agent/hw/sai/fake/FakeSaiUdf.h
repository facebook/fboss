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

#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class FakeUdf {
 public:
  FakeUdf(
      sai_object_id_t udfMatchId,
      sai_object_id_t udfGroupId,
      sai_int32_t base,
      sai_uint16_t offset)
      : udfMatchId(udfMatchId),
        udfGroupId(udfGroupId),
        base(base),
        offset(offset) {}

  sai_object_id_t id;
  sai_object_id_t udfMatchId;
  sai_object_id_t udfGroupId;
  sai_int32_t base;
  sai_uint16_t offset;
};

class FakeUdfMatch {
 public:
  FakeUdfMatch() = default;

  bool l2TypeEnable{false};
  sai_uint16_t l2TypeData;
  sai_uint16_t l2TypeMask;

  bool l3TypeEnable{false};
  sai_uint8_t l3TypeData;
  sai_uint8_t l3TypeMask;

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  bool l4DstPortTypeEnable{false};
  sai_uint16_t l4DstPortTypeData;
  sai_uint16_t l4DstPortTypeMask;
#endif

  sai_object_id_t id;
};

using FakeUdfManager = FakeManager<sai_object_id_t, FakeUdf>;
using FakeUdfMatchManager = FakeManager<sai_object_id_t, FakeUdfMatch>;

void populate_udf_api(sai_udf_api_t** udf_api);

} // namespace facebook::fboss
