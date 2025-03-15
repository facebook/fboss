#include "fboss/agent/test/utils/SystemScaleTestUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/utils/AclScaleTestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"

DECLARE_bool(intf_nbr_tables);

namespace {
constexpr int kNumMacs = 8000;
constexpr uint64_t kBaseMac = 0xFEEEC2000010;
} // namespace

namespace facebook::fboss::utility {

template <typename AddrT>
std::vector<std::pair<AddrT, folly::MacAddress>> neighborAddrs(
    int numNeighbors) {
  std::vector<std::pair<AddrT, folly::MacAddress>> macIPPairs;
  for (int i = 0; i < numNeighbors; ++i) {
    std::stringstream ipStream;

    if (std::is_same<AddrT, folly::IPAddressV6>::value) {
      ipStream << "2001:0db8:85a3:0000:0000:8a2e:" << std::hex
               << (i >> 16 & 0xffff) << ":" << std::hex << (i & 0xffff);
    } else {
      ipStream << "100.100." << (i >> 8 & 0xff) << "." << (i & 0xff);
    }
    AddrT ip(ipStream.str());
    uint64_t macBytes = kBaseMac + 1;
    folly::MacAddress mac = folly::MacAddress::fromHBO(macBytes);
    macIPPairs.push_back(std::make_pair(ip, mac));
  }
  return macIPPairs;
}

template <typename AddrT>
std::shared_ptr<facebook::fboss::SwitchState> updateNeighborEntry(
    AgentEnsemble* ensemble,
    const std::shared_ptr<SwitchState>& in,
    const PortDescriptor& port,
    const AddrT& addr,
    const folly::MacAddress& mac,
    const bool isIntfNbrTable,
    std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
  using NeighborTableT = typename std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;
  auto state = in->clone();
  NeighborTableT* neighborTable;
  auto intfID = ensemble->getProgrammedState()
                    ->getPorts()
                    ->getNodeIf(ensemble->masterLogicalInterfacePortIds()[0])
                    ->getInterfaceID();

  if (isIntfNbrTable) {
    neighborTable = state->getInterfaces()
                        ->getNode(intfID)
                        ->template getNeighborEntryTable<AddrT>()
                        ->modify(intfID, &state);
  } else {
    auto vlanID = ensemble->getProgrammedState()
                      ->getPorts()
                      ->getNodeIf(ensemble->masterLogicalInterfacePortIds()[0])
                      ->getIngressVlan();
    neighborTable = state->getVlans()
                        ->getNode(vlanID)
                        ->template getNeighborEntryTable<AddrT>()
                        ->modify(vlanID, &state);
  }

  if (neighborTable->getEntryIf(addr)) {
    neighborTable->updateEntry(
        addr, mac, port, intfID, NeighborState::REACHABLE, lookupClass);
  } else {
    neighborTable->addEntry(addr, mac, port, intfID);
    // Update entry to add classid if any
    neighborTable->updateEntry(
        addr, mac, port, intfID, NeighborState::REACHABLE, lookupClass);
  }
  return state;
}

template <typename AddrT>
void programNeighbor(
    AgentEnsemble* ensemble,
    const PortDescriptor& port,
    const AddrT& addr,
    const folly::MacAddress neighborMac,
    std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
  ensemble->applyNewState(
      [&](const std::shared_ptr<SwitchState>& in) {
        return updateNeighborEntry(
            ensemble,
            in,
            port,
            addr,
            neighborMac,
            FLAGS_intf_nbr_tables,
            lookupClass);
      },
      "program neighbor",
      false);
}

void programNeighbors(
    AgentEnsemble* ensemble,
    const PortDescriptor& port,
    std::optional<cfg::AclLookupClass> lookupClass) {
  int numNDPNeighbors, numARPNeighbors;
  auto asic =
      utility::checkSameAndGetAsic(ensemble->getHwAsicTable()->getL3Asics());

  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK4 ||
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK5) {
    numNDPNeighbors = 4000;
    numARPNeighbors = 4000;
  } else if (
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK3 ||
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK ||
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO) {
    numNDPNeighbors = 2000;
    numARPNeighbors = 2000;
  } else {
    numNDPNeighbors = 2000;
    numARPNeighbors = 2000;
  }
  XLOG(DBG2) << "Max NDP neighbors: " << numNDPNeighbors << " Max ARP neighbors"
             << numARPNeighbors;

  for (auto pair : neighborAddrs<folly::IPAddressV4>(numARPNeighbors)) {
    programNeighbor(ensemble, port, pair.first, pair.second, lookupClass);
  }
  for (auto pair : neighborAddrs<folly::IPAddressV6>(numNDPNeighbors)) {
    programNeighbor(ensemble, port, pair.first, pair.second, lookupClass);
  }
}

void configureMaxNeighborEntries(AgentEnsemble* ensemble) {
  programNeighbors(
      ensemble,
      PortDescriptor(ensemble->masterLogicalInterfacePortIds()[0]),
      std::nullopt);
}

void configureMaxMacEntries(AgentEnsemble* ensemble) {
  CHECK_GE(ensemble->masterLogicalInterfacePortIds().size(), 2);
  PortID txPort = PortID(ensemble->masterLogicalInterfacePortIds()[1]);
  PortID rxPort = PortID(ensemble->masterLogicalInterfacePortIds()[0]);

  auto vlan = ensemble->getProgrammedState()
                  ->getPorts()
                  ->getNodeIf(rxPort)
                  ->getIngressVlan();

  auto rxInterfaceMac = getInterfaceMac(
      ensemble->getProgrammedState(),
      ensemble->getProgrammedState()
          ->getPorts()
          ->getNodeIf(rxPort)
          ->getInterfaceID());

  std::vector<std::pair<folly::IPAddressV6, folly::MacAddress>> macIPv6Pairs;

  for (int i = 0; i < kNumMacs; ++i) {
    std::stringstream ipStream;
    ipStream << "2001:0db8:85a3:0000:0000:8a2e:0370:" << std::hex << i;
    folly::IPAddressV6 ip(ipStream.str());
    uint64_t macBytes = 0xFEEEC2000010;
    folly::MacAddress mac = folly::MacAddress::fromHBO(macBytes + i);
    macIPv6Pairs.push_back(std::make_pair(ip, mac));
  }
  const int srcPort = 47231;
  const int dstPort = 277;
  for (const auto& [ipV6, mac] : macIPv6Pairs) {
    auto txPacket = utility::makeTCPTxPacket(
        ensemble->getSw(),
        vlan,
        mac,
        // MacAddress::BROADCAST send from second last interface to last
        // if replaced by macLastInterface, then traffic won't flood
        rxInterfaceMac,
        ipV6,
        folly::IPAddressV6("2620:0:1cfe:face:b10c::4"),
        srcPort,
        dstPort);
    ensemble->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), txPort);
  }
}

void configureMaxRouteEntries(AgentEnsemble* ensemble) {
  auto switchIds = ensemble->getHwAsicTable()->getSwitchIDs();
  CHECK_EQ(switchIds.size(), 1);
  auto asic =
      utility::checkSameAndGetAsic(ensemble->getHwAsicTable()->getL3Asics());
  auto* sw = ensemble->getSw();
  // reserve 10 ecmp for scaling routes
  auto const kMaxEcmpGropus =
      utility::getMaxEcmpGroups(ensemble->getL3Asics()) *
          FLAGS_ecmp_resource_percentage / 100 -
      10;

  auto maxNumRoute =
      asic->getMaxRoutes().has_value() ? asic->getMaxRoutes().value() : 0;
  CHECK_GT(maxNumRoute, 0);

  auto routeGenerator = ScaleTestRouteScaleGenerator(sw->getState());
  auto allThriftRoutes = routeGenerator.allThriftRoutes();

  std::vector<RoutePrefixV6> ecmpPrefixes;
  utility::EcmpSetupTargetedPorts6 ecmpHelper(ensemble->getProgrammedState());
  std::vector<PortID> portIds = ensemble->masterLogicalInterfacePortIds();
  std::vector<PortDescriptor> portDescriptorIds;

  for (const auto& portId : portIds) {
    portDescriptorIds.push_back(PortDescriptor(portId));
  }

  std::vector<std::vector<PortDescriptor>> allCombinations =
      utility::generateEcmpGroupScale(portDescriptorIds, kMaxEcmpGropus);
  std::vector<flat_set<PortDescriptor>> nhopSets;
  for (const auto& combination : allCombinations) {
    nhopSets.emplace_back(combination.begin(), combination.end());
  }

  XLOG(DBG2) << "Max Ecmp Groups: " << kMaxEcmpGropus
             << " Default Ecmp Width: " << kDefaulEcmpWidth
             << " combination size: " << allCombinations.size()
             << " generated routes: " << allThriftRoutes.size()
             << " routes assigned to ecmp group: " << allCombinations.size();

  // ECMP routes + generated routes should be less than max routes
  CHECK_LE(allThriftRoutes.size() + allCombinations.size(), maxNumRoute);

  ensemble->applyNewState(
      [&portDescriptorIds,
       &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(
            in,
            flat_set<PortDescriptor>(
                std::make_move_iterator(portDescriptorIds.begin()),
                std::make_move_iterator(portDescriptorIds.end())));
      },
      "program ecmp",
      false);

  std::generate_n(
      std::back_inserter(ecmpPrefixes), kMaxEcmpGropus, [i = 0]() mutable {
        return RoutePrefixV6{
            folly::IPAddressV6(folly::to<std::string>(2401, "::", i++)), 128};
      });

  auto wrapper = sw->getRouteUpdater();
  ecmpHelper.programRoutes(&wrapper, nhopSets, ecmpPrefixes);

  auto platformType = sw->getPlatformType();
  if (!routeGenerator.isSupported(platformType)) {
    throw FbossError(
        "Route scale generator not supported for platform: ", platformType);
  }

  auto updater = ensemble->getSw()->getRouteUpdater();
  const RouterID kRid(0);
  auto syncFib =
      [&updater, kRid](
          const utility::RouteDistributionGenerator::ThriftRouteChunk& routes) {
        std::for_each(
            routes.begin(), routes.end(), [&updater, kRid](const auto& route) {
              updater.addRoute(kRid, ClientID::BGPD, route);
            });
        updater.program(
            {{{kRid, ClientID::BGPD}},
             RouteUpdateWrapper::SyncFibInfo::SyncFibType::IP_ONLY});
      };
  // Sync fib with same set of routes
  // TH doesn't support 75K route scale, the following is a work around
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK) {
    std::vector<UnicastRoute> quarterThriftRoutes = std::vector<UnicastRoute>(
        allThriftRoutes.begin(),
        allThriftRoutes.begin() + allThriftRoutes.size() / 4);
    syncFib(quarterThriftRoutes);
  } else {
    syncFib(allThriftRoutes);
  }
  int route_count = 0;
  auto countRoutes = [&route_count](RouterID, auto&) { ++route_count; };
  forAllRoutes(sw->getState(), countRoutes);
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK) {
    CHECK_GE(route_count, allThriftRoutes.size() / 4);
  } else {
    CHECK_GE(route_count, allThriftRoutes.size());
  }
}

void configureMaxAclEntries(AgentEnsemble* ensemble) {
  auto cfg = ensemble->getCurrentConfig();
  const auto maxAclEntries = getMaxAclEntries(ensemble->getL3Asics());
  XLOG(DBG2) << "Max Acl Entries: " << maxAclEntries;
  std::vector<cfg::AclTableQualifier> qualifiers =
      setAclQualifiers(AclWidth::SINGLE_WIDE);

  utility::addAclTable(&cfg, "aclTable0", 0 /* priority */, {}, qualifiers);
  addAclEntries(maxAclEntries, AclWidth::SINGLE_WIDE, &cfg, "aclTable0");

  ensemble->applyNewConfig(cfg);
}

cfg::SwitchConfig getSystemScaleTestSwitchConfiguration(
    const AgentEnsemble& ensemble) {
  FLAGS_sai_user_defined_trap = true;
  auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
  auto asic = utility::checkSameAndGetAsic(l3Asics);
  auto config = utility::oneL3IntfNPortConfig(
      ensemble.getSw()->getPlatformMapping(),
      asic,
      ensemble.masterLogicalPortIds(),
      ensemble.getSw()->getPlatformSupportsAddRemovePort(),
      asic->desiredLoopbackModes());

  utility::addAclTableGroup(
      &config, cfg::AclStage::INGRESS, utility::kDefaultAclTableGroupName());
  utility::addDefaultAclTable(config);
  // We don't want to set queue rate that limits the number of rx pkts
  utility::addCpuQueueConfig(
      config,
      ensemble.getL3Asics(),
      ensemble.isSai(),
      /* setQueueRate */ false);
  utility::setDefaultCpuTrafficPolicyConfig(
      config, ensemble.getL3Asics(), ensemble.isSai());
  return config;
};

void initSystemScaleTest(AgentEnsemble* ensemble) {
  configureMaxAclEntries(ensemble);
  configureMaxRouteEntries(ensemble);
  configureMaxMacEntries(ensemble);
  configureMaxNeighborEntries(ensemble);
}
} // namespace facebook::fboss::utility
