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

#include <vector>

extern "C" {
#include <sai.h>
}
namespace facebook::fboss {

class FakeHash {
 public:
  using NativeFields = std::vector<int32_t>;
  using UDFGroups = std::vector<sai_object_id_t>;
  void setNativeHashFields(NativeFields nativeHash) {
    nativeHashFields_ = std::move(nativeHash);
  }
  void setUdfGroups(UDFGroups udfGroups) {
    udfGroups_ = std::move(udfGroups);
  }
  const NativeFields& getNativeHashFields() const {
    return nativeHashFields_;
  }
  const UDFGroups& getUdfGroups() const {
    return udfGroups_;
  }
  sai_object_id_t id;

 private:
  NativeFields nativeHashFields_;
  UDFGroups udfGroups_;
};

using FakeHashManager = FakeManager<sai_object_id_t, FakeHash>;

void populate_hash_api(sai_hash_api_t** hash_api);

} // namespace facebook::fboss
