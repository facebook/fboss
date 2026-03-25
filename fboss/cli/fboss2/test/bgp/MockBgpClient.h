// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>
#include <map>
#include <string>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h" // NOLINT(misc-include-cleaner)
#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/routing/policy/if/gen-cpp2/policy_thrift_types.h"

namespace facebook::fboss {
using namespace facebook::neteng::fboss::bgp::thrift;
using namespace facebook::neteng::fboss::bgp_attr;
using namespace facebook::bgp::bgp_policy;
using facebook::neteng::routing::policy::thrift::TPolicyStats;

class MockBgpClient : public apache::thrift::ServiceHandler<TBgpService> {
 public:
  MOCK_METHOD(void, getBgpSessions, (std::vector<TBgpSession>&));
  MOCK_METHOD(void, getBgpLocalConfig, (TBgpLocalConfig&));
  MOCK_METHOD(void, getDrainState, (TBgpDrainState&));
  MOCK_METHOD(void, getAttributeStats, (TAttributeStats&));
  MOCK_METHOD(void, getEntryStats, (TEntryStats&));
  MOCK_METHOD(void, getPolicyStats, (TPolicyStats&));
  MOCK_METHOD(void, getOriginatedRoutes, (std::vector<TOriginatedRoute>&));
  MOCK_METHOD(
      void,
      getBgpNeighbors,
      (std::vector<TBgpSession>&, std::unique_ptr<std::vector<std::string>>));
  MOCK_METHOD(void, getPeerEgressStats, (std::vector<TPeerEgressStats>&));

  using networks = std::map<TIpPrefix, std::vector<TBgpPath>>&;
  MOCK_METHOD(
      void,
      getPrefilterReceivedNetworks2,
      (networks, std::unique_ptr<std::string>));
  MOCK_METHOD(
      void,
      getPostfilterReceivedNetworks2,
      (networks, std::unique_ptr<std::string>));
  MOCK_METHOD(
      void,
      getPrefilterAdvertisedNetworks2,
      (networks, std::unique_ptr<std::string>));
  MOCK_METHOD(
      void,
      getPostfilterAdvertisedNetworks2,
      (networks, std::unique_ptr<std::string>));
  MOCK_METHOD(void, getRibEntries, (std::vector<TRibEntry>&, TBgpAfi));
  MOCK_METHOD(void, getChangeListEntries, (std::vector<TRibEntry>&, TBgpAfi));
  MOCK_METHOD(void, getShadowRibEntries, (std::vector<TRibEntry>&, TBgpAfi));
  MOCK_METHOD(void, getRunningConfig, (std::string&));
  MOCK_METHOD(
      void,
      getRibPrefix,
      (std::vector<TRibEntry>&, std::unique_ptr<std::string>));
  MOCK_METHOD(
      void,
      getRibSubprefixes,
      (std::vector<TRibEntry>&, std::unique_ptr<std::string>));
  MOCK_METHOD(
      void,
      getRibEntriesForCommunity,
      (std::vector<TRibEntry>&, TBgpAfi, std::unique_ptr<std::string>));
  MOCK_METHOD(void, getBgpStreamSessions, (std::vector<TBgpStreamSession>&));
  MOCK_METHOD(
      void,
      getSubscriberNetworkInfo,
      (networks, int32_t, std::unique_ptr<std::string>));
  MOCK_METHOD(void, setDebugLevel, (TBgpDebugLevel));
};
} // namespace facebook::fboss
