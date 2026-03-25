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

#include "folly/json/dynamic.h"

#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace {
inline constexpr auto kGar = "--decode-gar-lbw-ext-comm";
}

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;
using neteng::fboss::bgp_attr::TAsPath;
using neteng::fboss::bgp_attr::TBgpCommunity;
using neteng::fboss::bgp_attr::TIpPrefix;

namespace {
// This function computes a computation by recursively looking for available
// samples ahead of an offset.
template <class T>
void computeCombinations(
    int offset,
    int k,
    std::vector<T>& currentCombination,
    std::vector<std::vector<T>>& combinations,
    const std::vector<T>& container) {
  if (k == 0) {
    combinations.push_back(currentCombination);
    return;
  }

  for (int i = offset; i <= container.size() - k; ++i) {
    currentCombination.push_back(container.at(i));
    computeCombinations(
        i + 1, k - 1, currentCombination, combinations, container);
    currentCombination.pop_back();
  }
}
} // namespace

struct CmdShowVersionTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::map<std::string, std::string>;
};

void cmdShowVersion(CmdShowVersionTraits::RetType& binaryVersion);
std::string getPackageId(
    const std::string& packageVersion,
    const std::string& packageInfo);

std::map<std::set<std::string>, std::string> getCommunitySet(
    const HostInfo& host);

const std::string printCommunities(
    const std::vector<TBgpCommunity>& peer_communities,
    const HostInfo& hostInfo,
    bool isMultiLine = false);
const std::string printFBnetCommunities(
    const std::vector<TBgpCommunity>& peer_communities);

/*
 * Get bgp community information from DesiredCommunity at FBNet
 * with input of the attribute, coppesponding value and aimed field
 * return a vector that contains all the records that meet the requirement
 */
std::vector<std::string> getSkynetData(
    std::string attributeValue,
    std::string searchAttribute,
    std::string field);

/**
 * get the community names by the input value
 * conduct name_to_value search at FBNet
 * based on the size of the values, will conduct different method
 * to allocate the community name
 */
const std::map<std::vector<std::string>, std::string> findCommunitiesByFBNet(
    std::vector<std::string> communityList);

// Get the names in doubleValueResult that have 2 values found
std::map<std::vector<std::string>, std::string> addDoubleValueCommunity(
    std::map<std::vector<std::string>, std::string> resultMap,
    std::vector<std::string>& communityList,
    std::map<std::string, std::string>& singleName,
    const std::vector<std::vector<std::string>>& doubleValueResult);

// Get names in singleValueResult if they only have 1 value,
std::map<std::vector<std::string>, std::string> addSingleValueCommunity(
    std::map<std::vector<std::string>, std::string> resultMap,
    std::vector<std::string>& communityList,
    std::map<std::string, std::string>& singleName,
    const std::vector<std::string>& singleValueResult);

// Store the values that are not valid to NA
std::map<std::vector<std::string>, std::string> addNullCommunity(
    std::map<std::vector<std::string>, std::string> resultMap,
    std::vector<std::string>& communityList);

const std::string printClusterList(const std::vector<long>& clusterList);
const std::string printExtCommunities(
    const std::vector<TBgpExtCommunity>& extCommunities);
const std::string printAsPath(const TAsPath& asPath);
const std::string printLocalPrefs(
    int localPref,
    const std::map<int, std::string>& mnemonics);
folly::dynamic getLocalBgpConfig(const HostInfo& host);
const std::map<int, std::string> getLocalPrefMnemonics(
    const HostInfo& hostInfo);
// Reset static caches for testing
void resetBgpMnemonicCaches();
const std::vector<std::string> printRoutesInformation(
    const std::map<TIpPrefix, std::vector<TBgpPath>>& networks,
    const HostInfo& hostInfo,
    bool showPolicy);
// BGP Neighbors printOutput
void printTimestamp(
    long timeToParse,
    const std::string& outputMessage,
    std::ostream& out);
timeval splitFractionalSecondsFromTimer(long timer);
void printAddPathCapability(
    const std::vector<TBgpAddPathNegotiated>& capabilities,
    std::ostream& out);
void printBgpCapabilities(const TBgpSessionDetail& details, std::ostream& out);
void printBgpNeighborsOutput(
    const std::vector<TBgpSession>& neighbors,
    std::ostream& out);
void printRouteHeader(std::string& out, bool detailed);
// void printAdjacencies(
//     std::map<std::string, std::vector<openr::thrift::AdjacencyDatabase>>&
//         neighbors,
//     std::ostream& out);
// void filterReceivedRoutes(
//     openr::thrift::ReceivedRouteFilter& filter,
//     const std::vector<std::string>& addrs,
//     std::vector<std::string> node,
//     std::vector<std::string> area);
// void filterAdvertisedRoutes(
//     openr::thrift::AdvertisedRouteFilter& filter,
//     const std::vector<std::string>& addrs,
//     const std::string& prefixType = "");
// void printReceivedRoutesDetail(
//     const std::vector<openr::thrift::ReceivedRouteDetail>& routes,
//     std::ostream& out);
// void printOpenrAdvertisedRoute(
//     const std::vector<openr::thrift::AdvertisedRoute>& routes,
//     std::ostream& out);
// void printAdvertisedRoutesDetail(
//     const std::vector<openr::thrift::AdvertisedRouteDetail>& routes,
//     std::ostream& out);
const std::string formatBytes(size_t n);
// void printOpenrReceivedRoutes(
//     const std::vector<openr::thrift::ReceivedRouteDetail>& routes,
//     const std::unordered_map<std::string, std::string>& tagNames,
//     std::ostream& out,
//     bool detailed,
//     bool tag2Name = false);
// Prints entries for bgp table commands
void printRIBEntries(
    std::ostream& out,
    TRibEntryWithHost& entries,
    bool detail = false,
    const std::string& originator = "",
    bool fbnetsource = false);
std::map<TIpPrefix, std::vector<TBgpPath>> filterNetworks(
    const std::map<TIpPrefix, std::vector<TBgpPath>>& networks,
    const std::vector<std::string>& prefixes);
// Find the routes that where advertised/received but not accepted by a peer
const std::map<TIpPrefix, std::vector<TBgpPath>> getRejectedNetworks(
    const std::map<TIpPrefix, std::vector<TBgpPath>>& receivedNetworks,
    const std::map<TIpPrefix, std::vector<TBgpPath>>& acceptedNetworks);
// Searches for link bandwidth in a list of ext-communities.
float getLinkBandwidthFromExtCommunities(
    const std::vector<TBgpCommunity>& extCommunities);
// This functions maps from std::map<TIpPrefix, TBgpPath> to
// std::map<TIpPrefix, std::vector<TBgpPath>>
const std::map<TIpPrefix, std::vector<TBgpPath>> vectorizePaths(
    const std::map<TIpPrefix, TBgpPath>& networks);

// This function gets all the combinations of a given container
// in k number of samples. The return value is a list with all the
// combinations.
//
// Example: container = [1, 2, 3], k = 2
// The formula to compute combinations C(n, k):
//    C(n, k) = n! / (k! * (n - k)!) = 3! / (2! * (3 - 2)!) = 3 combinations
//
// Result = {[1, 2], [1, 3], [2, 3]}
template <class T>
std::vector<std::vector<T>> getCombinations(
    const std::vector<T>& container,
    int k) {
  const int size = container.size();
  if (k > size || k < 0) {
    throw std::out_of_range("Number of samples (k) out of combination range.");
  }
  if (k == size) {
    return {container};
  }
  if (k == 0) {
    return {};
  }

  std::vector<T> currentCombination;
  std::vector<std::vector<T>> combinations;
  computeCombinations(0, k, currentCombination, combinations, container);
  return combinations;
}

// Function to sort entries by prefix and next hop.
void sortEntries(std::vector<TRibEntry>& entries);
// Return communities for a given community mnemonic.
const std::vector<std::string> communityNameToValue(
    const std::string& communityName,
    const HostInfo& hostInfo);
// Returns asn:value from community
const std::string communityToString(const TBgpCommunity& community);
// Checks for a given community that the asn and value are integers
bool isValidCommunity(std::string_view communityName);

void stringFormatTimestamp(
    time_t timestamp,
    const char* format,
    std::string* output);

} // namespace facebook::fboss
