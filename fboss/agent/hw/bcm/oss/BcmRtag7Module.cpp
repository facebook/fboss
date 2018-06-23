/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmRtag7Module.h"
#include "fboss/agent/FbossError.h"

namespace facebook {
namespace fboss {

int BcmRtag7Module::getFlowLabelSubfields() const {
  throw FbossError("Flow label symbols not exported by OpenNSL");
  return 0;
}

void BcmRtag7Module::enableFlowLabelSelection() {
  throw FbossError("Flow label symbols not exported by OpenNSL");
}

} // namespace fboss
} // namespace facebook
