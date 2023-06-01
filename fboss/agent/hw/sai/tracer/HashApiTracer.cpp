/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/HashApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/HashApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _HashMap{
    SAI_ATTR_MAP(Hash, NativeHashFieldList),
    SAI_ATTR_MAP(Hash, UDFGroupList),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(hash, SAI_OBJECT_TYPE_HASH, hash);
WRAP_REMOVE_FUNC(hash, SAI_OBJECT_TYPE_HASH, hash);
WRAP_SET_ATTR_FUNC(hash, SAI_OBJECT_TYPE_HASH, hash);
WRAP_GET_ATTR_FUNC(hash, SAI_OBJECT_TYPE_HASH, hash);

sai_hash_api_t* wrappedHashApi() {
  static sai_hash_api_t hashWrappers;

  hashWrappers.create_hash = &wrap_create_hash,
  hashWrappers.remove_hash = &wrap_remove_hash;
  hashWrappers.set_hash_attribute = &wrap_set_hash_attribute;
  hashWrappers.get_hash_attribute = &wrap_get_hash_attribute;

  return &hashWrappers;
}

SET_SAI_ATTRIBUTES(Hash)

} // namespace facebook::fboss
