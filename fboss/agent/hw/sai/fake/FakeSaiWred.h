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

class FakeWred {
 public:
  explicit FakeWred(
      bool greenEnable,
      sai_uint32_t greenMinThreshold,
      sai_uint32_t greenMaxThreshold,
      sai_int32_t ecnMarkMode,
      sai_uint32_t ecnGreenMinThreshold,
      sai_uint32_t ecnGreenMaxThreshold)
      : greenEnable_(greenEnable),
        greenMinThreshold_(greenMinThreshold),
        greenMaxThreshold_(greenMaxThreshold),
        ecnMarkMode_(ecnMarkMode),
        ecnGreenMinThreshold_(ecnGreenMinThreshold),
        ecnGreenMaxThreshold_(ecnGreenMaxThreshold) {}

  void setGreenEnable(bool enable) {
    greenEnable_ = enable;
  }
  bool getGreenEnable() {
    return greenEnable_;
  }
  sai_uint32_t getGreenMinThreshold() {
    return greenMinThreshold_;
  }
  void setGreenMinThreshold(sai_uint32_t greenMinThreshold) {
    greenMinThreshold_ = greenMinThreshold;
  }
  sai_uint32_t getGreenMaxThreshold() {
    return greenMaxThreshold_;
  }
  void setGreenMaxThreshold(sai_uint32_t greenMaxThreshold) {
    greenMaxThreshold_ = greenMaxThreshold;
  }
  sai_int32_t getEcnMarkMode() {
    return ecnMarkMode_;
  }
  void setEcnMarkMode(sai_int32_t ecnMarkMode) {
    ecnMarkMode_ = ecnMarkMode;
  }
  sai_uint32_t getEcnGreenMinThreshold() {
    return ecnGreenMinThreshold_;
  }
  void setEcnGreenMinThreshold(sai_uint32_t ecnGreenMinThreshold) {
    ecnGreenMinThreshold_ = ecnGreenMinThreshold;
  }
  sai_uint32_t getEcnGreenMaxThreshold() {
    return ecnGreenMaxThreshold_;
  }
  void setEcnGreenMaxThreshold(sai_uint32_t ecnGreenMaxThreshold) {
    ecnGreenMaxThreshold_ = ecnGreenMaxThreshold;
  }

  sai_object_id_t id;

 private:
  bool greenEnable_{false};
  sai_uint32_t greenMinThreshold_{0};
  sai_uint32_t greenMaxThreshold_{0};
  sai_int32_t ecnMarkMode_{SAI_ECN_MARK_MODE_NONE};
  sai_uint32_t ecnGreenMinThreshold_{0};
  sai_uint32_t ecnGreenMaxThreshold_{0};
};

using FakeWredManager = FakeManager<sai_object_id_t, FakeWred>;

void populate_wred_api(sai_wred_api_t** wred_api);

} // namespace facebook::fboss
