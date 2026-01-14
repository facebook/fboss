/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/QsfpServiceThreads.h"

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {

std::shared_ptr<QsfpServiceThreads> createQsfpServiceThreads(
    const std::shared_ptr<const PlatformMapping>& platformMapping) {
  auto qsfpServiceThreads = std::make_shared<QsfpServiceThreads>();

  for (const auto& tcvrID :
       utility::getTransceiverIds(platformMapping->getChips())) {
    qsfpServiceThreads->transceiverToThread.emplace(
        tcvrID, SlotThreadHelper(tcvrID));
  }

  return qsfpServiceThreads;
}

} // namespace facebook::fboss
