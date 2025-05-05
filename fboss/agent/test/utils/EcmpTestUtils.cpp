/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/EcmpTestUtils.h"

namespace facebook::fboss::utility {

std::map<int32_t, int32_t> getEcmpWeightsInHw(
    AgentEnsemble* ensemble,
    facebook::fboss::utility::CIDRNetwork cidr) {
  auto switchId = ensemble->getSw()
                      ->getScopeResolver()
                      ->scope(ensemble->masterLogicalPortIds()[0])
                      .switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);
  std::map<int32_t, int32_t> ecmpWeights;
  client->sync_getEcmpWeights(ecmpWeights, cidr, 0);
  return ecmpWeights;
}

int getEcmpSizeInHw(
    AgentEnsemble* ensemble,
    facebook::fboss::utility::CIDRNetwork cidr) {
  auto ecmpWeights = getEcmpWeightsInHw(ensemble, cidr);
  int ecmpSize = 0;
  for (const auto& [_, weight] : ecmpWeights) {
    ecmpSize += weight;
  }
  return ecmpSize;
}

} // namespace facebook::fboss::utility
