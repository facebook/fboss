// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fmt/core.h>
#include <iostream>

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {
using facebook::neteng::fboss::bgp::thrift::TAttributeStats;

struct CmdShowBgpStatsAttrsTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = TAttributeStats;
};

class CmdShowBgpStatsAttrs
    : public CmdHandler<CmdShowBgpStatsAttrs, CmdShowBgpStatsAttrsTraits> {
 public:
  using RetType = CmdShowBgpStatsAttrsTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    TAttributeStats stats;
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
    client->sync_getAttributeStats(stats);
    return stats;
  }

  void printOutput(const RetType& stats, std::ostream& out = std::cout) {
    out << "BGP attribute statistics:" << std::endl;
    out << " Total number of attributes: "
        << folly::copy(stats.total_num_of_attributes().value()) << std::endl;
    out << " Total number of unique attributes: "
        << folly::copy(stats.total_unique_attributes().value()) << std::endl;
    out << fmt::format(
        " Average attribute reference count: {:.2f}\n",
        folly::copy(stats.avg_attribute_refcount().value()));
    out << fmt::format(
        " Average community list length: {:.2f}\n",
        folly::copy(stats.avg_community_list_len().value()));
    out << fmt::format(
        " Average extended community list length: {:.2f}\n",
        folly::copy(stats.avg_extcommunity_list_len().value()));
    out << fmt::format(
        " Average as path length: {:.2f}\n",
        folly::copy(stats.avg_as_path_len().value()));
    out << fmt::format(
        " Average cluster list length: {:.2f}\n",
        folly::copy(stats.avg_cluster_list_len().value()));
    out << fmt::format(
        " Average topology info length: {:.2f}\n",
        folly::copy(stats.avg_topology_info_len().value()));
  }
};
} // namespace facebook::fboss
