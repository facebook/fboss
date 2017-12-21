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

#include <vector>

extern "C" {
#include <opennsl/types.h>
}

namespace facebook { namespace fboss {

struct CosQueueGports {
  opennsl_gport_t scheduler;
  std::vector<opennsl_gport_t> unicast;
  std::vector<opennsl_gport_t> multicast;
};

class BcmCosManager {
public:
  BcmCosManager() {}
  // This is to enable RTTI so that we can downcast.
  virtual ~BcmCosManager() {}
};

}}
