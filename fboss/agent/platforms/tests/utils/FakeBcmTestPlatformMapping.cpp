/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/tests/utils/FakeBcmTestPlatformMapping.h"

namespace {
using namespace facebook::fboss;
static const std::array<int, 8> kMasterLogicalPortIds =
    {1, 5, 9, 13, 17, 21, 25, 29};

static const std::unordered_map<int, std::vector<cfg::PortProfileID>> kLanes = {
    {0,
     {
         cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91,
         cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC,
         cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
         cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
         cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
     }},
    {1,
     {
         cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
         cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
     }},
    {2,
     {
         cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC,
         cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
         cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
     }},
    {3,
     {
         cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
         cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
     }},
};

static const std::
    unordered_map<cfg::PortProfileID, std::pair<cfg::PortSpeed, int>>
        kProfiles = {
            {cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91,
             std::make_pair(cfg::PortSpeed::HUNDREDG, 4)},
            {cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC,
             std::make_pair(cfg::PortSpeed::FIFTYG, 2)},
            {cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
             std::make_pair(cfg::PortSpeed::FORTYG, 4)},
            {cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
             std::make_pair(cfg::PortSpeed::TWENTYFIVEG, 1)},
            {cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
             std::make_pair(cfg::PortSpeed::XG, 1)},
};
} // namespace

namespace facebook {
namespace fboss {
FakeBcmTestPlatformMapping::FakeBcmTestPlatformMapping() : PlatformMapping() {
  for (auto itProfile : kProfiles) {
    phy::PortProfileConfig profile;
    profile.speed = itProfile.second.first;
    profile.iphy.numLanes = itProfile.second.second;
    setSupportedProfile(itProfile.first, profile);
  }

  for (auto controllingPort : kMasterLogicalPortIds) {
    for (auto itLane : kLanes) {
      auto portID = controllingPort + itLane.first;
      cfg::PlatformPortEntry port;
      port.mapping.id = portID;
      port.mapping.controllingPort = controllingPort;
      for (auto profile : itLane.second) {
        cfg::PlatformPortConfig portCfg;
        if (auto numLane = kProfiles.find(profile)->second.second;
            numLane > 1) {
          std::vector<int32_t> subsumedPorts;
          for (int i = 1; i < numLane; i++) {
            subsumedPorts.push_back(portID + i);
          }
          portCfg.subsumedPorts_ref() = subsumedPorts;
        }
        port.supportedProfiles.emplace(profile, portCfg);
      }
      setPlatformPort(portID, port);
    }
  }
  CHECK(
      getPlatformPorts().size() ==
      kMasterLogicalPortIds.size() * kLanes.size());
}
} // namespace fboss
} // namespace facebook
