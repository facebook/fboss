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
#include "fboss/agent/hw/sai/tracer/Utils.h"

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

void setHashAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_HASH_ATTR_NATIVE_HASH_FIELD_LIST:
        s32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_HASH_ATTR_UDF_GROUP_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
