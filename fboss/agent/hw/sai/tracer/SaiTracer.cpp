/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"
#include "fboss/agent/hw/sai/tracer/AclApiTracer.h"

#include <folly/Singleton.h>

extern "C" {
#include <sai.h>
}

DEFINE_bool(
    enable_replayer,
    false,
    "Flag to indicate whether function wrapper and logger is enabled.");

extern "C" {

// Real functions for Sai APIs
sai_status_t __real_sai_api_query(
    sai_api_t sai_api_id,
    void** api_method_table);

// Wrap function for Sai APIs
sai_status_t __wrap_sai_api_query(
    sai_api_t sai_api_id,
    void** api_method_table) {
  // Functions defined in sai.h can be properly wrapped using the linker trick
  // e.g. sai_api_query(), sai_api_initialize() ...
  // However, for other functions such as create_*, set_*, and get_*,
  // their function pointers are returned to the api_method_table in this call.
  // To 'wrap' those function pointers as well, we store the real
  // function pointers and return the 'wrapped' function pointers defined in
  // this tracer directory, adding an redirection layer for logging purpose.
  // (See 'AclApiTracer.h' for example).

  sai_status_t rv = __real_sai_api_query(sai_api_id, api_method_table);

  if (!FLAGS_enable_replayer) {
    return rv;
  }

  switch (sai_api_id) {
    case (SAI_API_ACL):
      facebook::fboss::SaiTracer::getInstance()->aclApi_ =
          static_cast<sai_acl_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapAclApi();
      break;
    default:
      // TODO: For other APIs, create new API wrappers and invoke wrapApi()
      // funtion here
      break;
  }
  return rv;
}

} // extern "C"

namespace {

folly::Singleton<facebook::fboss::SaiTracer> _saiTracer;

}

namespace facebook::fboss {

std::shared_ptr<SaiTracer> SaiTracer::getInstance() {
  return _saiTracer.try_get();
}

} // namespace facebook::fboss
