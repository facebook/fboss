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

#include <memory>

#include <gflags/gflags.h>

extern "C" {
#include <sai.h>
}

DECLARE_bool(enable_replayer);

namespace facebook::fboss {

class SaiTracer {
 public:
  static std::shared_ptr<SaiTracer> getInstance();

  sai_acl_api_t* aclApi_;

 private:
};

} // namespace facebook::fboss
