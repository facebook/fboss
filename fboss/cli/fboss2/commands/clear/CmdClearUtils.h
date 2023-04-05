// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

namespace facebook::fboss::utils {

template <typename NeighborEntry>
std::string flushNeighborEntries(
    std::unique_ptr<apache::thrift::Client<FbossCtrl>>& agent,
    const std::vector<NeighborEntry>& allNeighbors,
    const utils::CIDRNetwork& networkFilter) {
  static_assert(
      std::is_same_v<NeighborEntry, NdpEntryThrift> ||
      std::is_same_v<NeighborEntry, ArpEntryThrift>);

  std::string nbrTypeStr;
  if constexpr (std::is_same_v<NeighborEntry, NdpEntryThrift>) {
    nbrTypeStr = "Ndp";
  } else {
    nbrTypeStr = "Arp";
  }

  std::vector<NeighborEntry> entriesToFlush;

  for (auto const& entry : allNeighbors) {
    auto ip = folly::IPAddress::fromBinary(
        folly::ByteRange(folly::StringPiece(entry.get_ip().get_addr())));
    auto shouldFlush = networkFilter.empty() ||
        std::find_if(
            networkFilter.begin(),
            networkFilter.end(),
            [&ip](const auto& network) {
              return ip.inSubnet(network.first, network.second);
            }) != networkFilter.end();
    if (shouldFlush) {
      entriesToFlush.emplace_back(entry);
    }
  }

  for (const auto& entry : entriesToFlush) {
    auto ip = folly::IPAddress::fromBinary(
        folly::ByteRange(folly::StringPiece(entry.get_ip().get_addr())));
    std::cout << fmt::format(
        "Deleting {} entry ip: {} vlanID: {}\n",
        nbrTypeStr,
        ip.str(),
        entry.get_vlanID());
    agent->sync_flushNeighborEntry(entry.get_ip(), entry.get_vlanID());
  }

  return folly::to<std::string>("Flushed ", entriesToFlush.size(), " entries");
}

} // namespace facebook::fboss::utils
