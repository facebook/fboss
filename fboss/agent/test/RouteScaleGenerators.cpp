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
#include "fboss/agent/test/ResourceLibUtil.h"

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

/*
 * Double UU scale plus 4K /64 to give us just over ~100K prefixes.
 * This is our anticupated worst case route scale in the near to mid
 * term
 */
AnticipatedRouteScaleGenerator::AnticipatedRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : RouteDistributionGenerator(
          startingState,
          // v6 distribution
          {
              {127, 256},
              {128, 2452},
              {24, 2},
              {37, 74},
              {44, 36},
              {46, 2096},
              {47, 16},
              {48, 50},
              {52, 608},
              {54, 32},
              {56, 1536},
              {57, 272},
              {59, 1540},
              {60, 1566},
              {61, 56},
              {62, 480},
              {63, 4182},
              {64, 50786},
          },
          // v4 distribution
          {

              {19, 16},
              {21, 2},
              {24, 304},
              {27, 832},
              {31, 256},
              {32, 33250},
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
              // ip2ip routes. There may not be any in turbo fabric
              // adding few just to test the code.
              {46, 12},
              {56, 12},
              {64, 12},
              {128, 11},
          },
          // v4 distribution
          {
              {26, 11},
              {32, 11},
          },
          chunkSize,
          ecmpWidth,
          routerId),
      // V6 Routes per label path
      v6PrefixLabelDistributionSpec_({
          // mapping from prefix len to
          // {numlabelledRoutes, numRoutesPerLabel, startingLabel}
          //
          // 11 pods within mesh + 84 pods outside mesh
          {46,
           {95, 8, 100}}, // 11 interpod + 1 spine = 12 ECMP paths share routes
          // 11 pods within mesh + 84 pods outside mesh
          {56,
           {95, 8, 100}}, // 11 interpod + 1 spine = 12 ECMP paths share routes
          // 11 pods within mesh + 3750 VIP routes
          {64, {3761, 376, 200}}, // 10 spine ECMP NHs in link failure cases.
                                  // In steady state, the routes will resolve
                                  // over a single ECMP NH
          {128, {11, 1, 300}}, // 11 spines
      }),
      // V4 Routes per label path
      // 11 /26 for interpod + 3750 VIP routes
      v4PrefixLabelDistributionSpec_(
          {{26, {11, 1, 500}}, {32, {3761, 376, 600}}}) {
  boost::container::flat_set<PortDescriptor> allPorts;
  for (auto port : std::as_const(*startingState->getPorts())) {
    if (!port.second->isEnabled()) {
      continue;
    }
    allPorts.insert(PortDescriptor(port.second->getID()));
  }
  CHECK_GE(allPorts.size(), ecmpWidth);

  size_t unlabeledPortsSize = ecmpWidth - 32;
  for (auto i = 0; i < ecmpWidth; i++) {
    auto iPortId = *(allPorts.begin() + i);
    if (i < unlabeledPortsSize) {
      unlabeledPorts_.insert(iPortId);
    } else {
      labeledPorts_.insert(iPortId);
    }
  }
}

template <typename AddrT>
void TurboFSWRouteScaleGenerator::genIp2MplsRouteDistribution(
    const MaskLen2PrefixLabelDistribution& labelDistributionSpec,
    const boost::container::flat_set<PortDescriptor>& labeledPorts) const {
  using PrefixT = RoutePrefix<AddrT>;
  std::vector<PrefixT> prefixes;
  EcmpSetupTargetedPorts<AddrT> ecmpHelper(startingState());

  for (const auto& prefixLabelDistribution : labelDistributionSpec) {
    uint8_t prefixSize = prefixLabelDistribution.first;
    uint32_t numRoutes = prefixLabelDistribution.second.totalPrefixes;
    uint32_t chunkSize = prefixLabelDistribution.second.chunkSize;
    uint32_t labelForChunk = prefixLabelDistribution.second.startingLabel;

    auto prefixGenerator = PrefixGenerator<AddrT>(prefixSize);
    while (numRoutes) {
      int label = labelForChunk;
      for (auto j = 0; j < chunkSize && numRoutes > 0; ++j, numRoutes--) {
        const auto cidrNetwork =
            getNewPrefix(prefixGenerator, startingState(), getRouterID());
        if constexpr (std::is_same<folly::IPAddressV6, AddrT>::value) {
          prefixes.emplace_back(
              RoutePrefix<AddrT>{cidrNetwork.first.asV6(), cidrNetwork.second});
        } else {
          prefixes.emplace_back(
              RoutePrefix<AddrT>{cidrNetwork.first.asV4(), cidrNetwork.second});
        }
      }

      // b19 is always 1, b18 identifies IP version
      if constexpr (std::is_same<folly::IPAddressV6, AddrT>::value) {
        label = 0x3 << 18 | ((0xff & label) << 10);
      } else {
        label = 0x2 << 18 | ((0xff & label) << 10);
      }
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> labels{};
      for (auto i = 0; i < labeledPorts.size(); i++) {
        auto labeledPort = *(labeledPorts.begin() + i);
        LabelForwardingAction::LabelStack stack{
            label + static_cast<int>(labeledPort.phyPortID())};
        labels.emplace(labeledPort, stack);
      }
      RouteChunk routeChunk;

      for (auto& prefix : prefixes) {
        Route route;
        route.prefix = prefix.toCidrNetwork();
        // create ucmp with 1:3 weight distribution for labelled paths
        for (auto port : unlabeledPorts_) {
          UnresolvedNextHop nhop(ecmpHelper.ip(port), ECMP_WEIGHT);
          route.nhops.push_back(nhop);
        }
        for (auto port : labeledPorts_) {
          UnresolvedNextHop nhop(
              ecmpHelper.ip(port),
              3,
              LabelForwardingAction(
                  LabelForwardingAction::LabelForwardingType::PUSH,
                  labels.find(port)->second));
          route.nhops.push_back(nhop);
        }
        routeChunk.push_back(route);
      }
      generatedRouteChunks_->push_back(routeChunk);
      labelForChunk++;
      prefixes.clear();
    }
  }
}

void TurboFSWRouteScaleGenerator::genRoutes() const {
  if (generatedRouteChunks_) {
    return;
  }
  // Add ip2ip routes
  RouteDistributionGenerator::genRoutes();
  // Add v6 labelled routes
  genIp2MplsRouteDistribution<folly::IPAddressV6>(
      v6PrefixLabelDistributionSpec_, labeledPorts_);

  // Add v4 labelled routes
  genIp2MplsRouteDistribution<folly::IPAddressV4>(
      v4PrefixLabelDistributionSpec_, labeledPorts_);
}

std::shared_ptr<SwitchState> TurboFSWRouteScaleGenerator::resolveNextHops(
    std::shared_ptr<SwitchState> in) const {
  auto ecmpHelper4 = EcmpSetupTargetedPorts4(in);
  auto ecmpHelper6 = EcmpSetupTargetedPorts6(in);
  auto nhopsResolvedState = ecmpHelper6.resolveNextHops(in, unlabeledPorts_);
  nhopsResolvedState =
      ecmpHelper6.resolveNextHops(nhopsResolvedState, labeledPorts_);
  nhopsResolvedState =
      ecmpHelper4.resolveNextHops(nhopsResolvedState, unlabeledPorts_);
  nhopsResolvedState =
      ecmpHelper4.resolveNextHops(nhopsResolvedState, labeledPorts_);
  return nhopsResolvedState;
}

bool TurboFSWRouteScaleGenerator::isSupported(PlatformMode mode) const {
  return (
      mode == PlatformMode::MINIPACK || mode == PlatformMode::YAMP ||
      mode == PlatformMode::FUJI || mode == PlatformMode::ELBERT);
}
} // namespace facebook::fboss::utility
