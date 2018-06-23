/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmRtag7LoadBalancer.h"
#include "fboss/agent/FbossError.h"

namespace facebook {
namespace fboss {

opennsl_switch_control_t BcmRtag7LoadBalancer::trunkHashSet0UnicastOffset()
    const {
  throw FbossError("Symbol not exported in OpenNSL");
}

} // namespace fboss
} // namespace facebook
