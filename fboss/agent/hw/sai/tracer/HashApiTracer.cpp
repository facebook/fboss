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

sai_status_t wrap_create_hash(
    sai_object_id_t* hash_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->hashApi_->create_hash(
      hash_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_hash",
      hash_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_HASH,
      rv);
  return rv;
}

sai_status_t wrap_remove_hash(sai_object_id_t hash_id) {
  auto rv = SaiTracer::getInstance()->hashApi_->remove_hash(hash_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_hash", hash_id, SAI_OBJECT_TYPE_HASH, rv);
  return rv;
}

sai_status_t wrap_set_hash_attribute(
    sai_object_id_t hash_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->hashApi_->set_hash_attribute(hash_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_hash_attribute", hash_id, attr, SAI_OBJECT_TYPE_HASH, rv);
  return rv;
}

sai_status_t wrap_get_hash_attribute(
    sai_object_id_t hash_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->hashApi_->get_hash_attribute(
      hash_id, attr_count, attr_list);
}

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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
