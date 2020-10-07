/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/RouteScaleGenerators.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace facebook::fboss::utility {

/*
 * RSW distribution was discussed here
 * https://fb.workplace.com/groups/266410803370065/permalink/3170120682999048/
 * There are 2 changes to this distritbution here.
 * i) We found /128 did not factor in pod local RSW loopbacks. So /128 should
 * have been 49 instead of 1. To give some room, I have doubled them to be 100.
 * ii) We increased static routes for ILA/IP per task from 384 to 1024 as part
 * of S185053, so upping the scale limits here too.
 */
RSWRouteScaleGenerator::RSWRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : RouteDistributionGenerator(
          startingState,
          // v6 distribution
          {
              {46, 96},
              {54, 624},
              {66, 96},
              {57, 16},
              {59, 96},
              {60, 96},
              {64, 3718},
              {127, 128},
              {128, 100},
          },
          // v4 distribution
          {
              {19, 80},
              {24, 592},
              {26, 1},
              {31, 128},
              {32, 2176},
          },
          chunkSize,
          ecmpWidth,
          routerId) {}

FSWRouteScaleGenerator::FSWRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : RouteDistributionGenerator(
          startingState,
          // v6 distribution
          {
              {48, 100},
              {52, 200},
              {56, 100},
              {64, 3550},
              {80, 300},
              {96, 200},
              {112, 100},
              {127, 100},
              {128, 3350},
          },
          // v4 distribution
          {
              {15, 200},
              {24, 2000},
              {26, 1000},
              {28, 200},
              {31, 100},
              {32, 4500},
          },
          chunkSize,
          ecmpWidth,
          routerId) {}

THAlpmRouteScaleGenerator::THAlpmRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : RouteDistributionGenerator(
          startingState,
          // v6 distribution
          {
              {48, 200},
              {52, 200},
              {56, 200},
              {64, 10000},
              {80, 200},
              {96, 200},
              {112, 200},
              {120, 200},
              {128, 10000},
          },
          // v4 distribution
          {
              {15, 400},
              {24, 400},
              {26, 400},
              {28, 400},
              {30, 400},
              {32, 10000},
          },
          chunkSize,
          ecmpWidth,
          routerId) {}

HgridDuRouteScaleGenerator::HgridDuRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : RouteDistributionGenerator(
          startingState,
          // v6 distribution
          {
              {37, 8},
              {47, 8},
              {46, 768},
              {52, 256},
              {54, 1},
              {56, 768},
              {57, 2},
              {59, 768},
              {60, 768},
              {64, 16344},
              {127, 128},
              {128, 1},
          },
          // v4 distribution
          {

              {19, 1},
              {24, 99},
              {26, 96},
              {27, 384},
              {31, 128},
              {32, 16721},
          },
          chunkSize,
          ecmpWidth,
          routerId) {}

HgridUuRouteScaleGenerator::HgridUuRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : RouteDistributionGenerator(
          startingState,
          // v6 distribution
          {
              {127, 128},
              {128, 1226},
              {24, 1},
              {37, 37},
              {44, 18},
              {46, 1048},
              {47, 8},
              {48, 25},
              {52, 304},
              {54, 16},
              {56, 768},
              {57, 136},
              {59, 770},
              {60, 783},
              {61, 28},
              {62, 240},
              {63, 2091},
              {64, 23393},
          },
          // v4 distribution
          {

              {19, 8},
              {21, 1},
              {24, 152},
              {27, 416},
              {31, 128},
              {32, 16625},
          },
          chunkSize,
          ecmpWidth,
          routerId) {}

TurboFSWRouteScaleGenerator::TurboFSWRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : RouteDistributionGenerator(
          startingState,
          {
              {
                  46,
                  (95 + // Pod Aggregates
                   11 + // Pods Within the Mesh
                   84) // Pods Outside the Mesh
              },
              {
                  56,
                  (95 + // Pod Aggregates
                   11 + // Pods Within the Mesh
                   84) // Pods Outside the Mesh
              },
              {64, 11}, // Pod RSW Loopback Aggregates
              {128, 11}, // FSW Loopbacks
          },
          // v4 distribution
          {
              {26, 11}, // Pod RSW Loopback Aggregates
              {32, 11}, // FSW Loopbacks
          },
          chunkSize,
          ecmpWidth,
          routerId) {}

const RouteDistributionGenerator::SwitchStates&
TurboFSWRouteScaleGenerator::getSwitchStates() const {
  auto& generatedStates = getGeneratedStates();
  if (generatedStates) {
    return *generatedStates;
  }
  generatedStates = SwitchStates();
  auto state = startingState();

  auto ecmpHelper4 = EcmpSetupTargetedPorts4(state);
  auto ecmpHelper6 = EcmpSetupTargetedPorts6(state);
  auto width = ecmpWidth();

  CHECK_GE(width, 32);
  size_t unlabeledPortsSize = width - 32;

  boost::container::flat_set<PortDescriptor> unlabeledPorts{};
  boost::container::flat_set<PortDescriptor> labeledPorts{};
  boost::container::flat_set<PortDescriptor> allPorts{};

  for (auto port : *state->getPorts()) {
    if (!port->isEnabled()) {
      continue;
    }
    allPorts.insert(PortDescriptor(port->getID()));
  }
  CHECK_GE(allPorts.size(), width);

  for (auto i = 0; i < width; i++) {
    auto iPortId = *(allPorts.begin() + i);
    if (i < unlabeledPortsSize) {
      unlabeledPorts.insert(iPortId);
    } else {
      labeledPorts.insert(iPortId);
    }
  }

  auto nhopsResolvedState =
      ecmpHelper6.resolveNextHops(startingState(), unlabeledPorts);
  nhopsResolvedState =
      ecmpHelper6.resolveNextHops(nhopsResolvedState, labeledPorts);
  nhopsResolvedState =
      ecmpHelper4.resolveNextHops(nhopsResolvedState, unlabeledPorts);
  nhopsResolvedState =
      ecmpHelper4.resolveNextHops(nhopsResolvedState, labeledPorts);
  nhopsResolvedState->publish();
  generatedStates->push_back(nhopsResolvedState);

  // b19 is always 1, b18 identifies IP version
  int v4LabelBase = 0x3 << 18;
  int v6LabelBase = 0x2 << 18;
  // [b17-b10] (8 bits) identify prefix
  // maximum 256 prefix possible
  int prefixCountV4 = 1;
  int prefixCountV6 = 1;
  for (const auto& routeChunk : get()) {
    auto newState = generatedStates->back()->clone();
    for (const auto& route : routeChunk) {
      std::vector<RoutePrefixV6> v6Prefixes;
      std::vector<RoutePrefixV4> v4Prefixes;
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> labels{};
      const auto& cidrNetwork = route.prefix;
      int label = cidrNetwork.first.isV6()
          ? v6LabelBase | ((0xff & prefixCountV6) << 10)
          : v4LabelBase | ((0xff & prefixCountV4) << 10);

      for (auto i = 0; i < labeledPorts.size(); i++) {
        auto labeledPort = *(labeledPorts.begin() + i);
        LabelForwardingAction::LabelStack stack{
            label + static_cast<int>(labeledPort.phyPortID())};
        labels.emplace(labeledPort, stack);
      }
      if (cidrNetwork.first.isV6()) {
        prefixCountV6++;
        v6Prefixes.emplace_back(
            RoutePrefixV6{cidrNetwork.first.asV6(), cidrNetwork.second});
        newState = ecmpHelper6.setupIp2MplsECMPForwarding(
            newState, allPorts, labels, v6Prefixes);
      } else {
        prefixCountV4++;
        v4Prefixes.emplace_back(
            RoutePrefixV4{cidrNetwork.first.asV4(), cidrNetwork.second});
        newState = ecmpHelper4.setupIp2MplsECMPForwarding(
            newState, allPorts, labels, v4Prefixes);
      }
    }
    generatedStates->push_back(newState);
  }

  return *generatedStates;
}

bool TurboFSWRouteScaleGenerator::isSupported(PlatformMode mode) const {
  return (
      mode == PlatformMode::MINIPACK || mode == PlatformMode::YAMP ||
      mode == PlatformMode::WEDGE400 || mode == PlatformMode::WEDGE400C);
}
} // namespace facebook::fboss::utility
