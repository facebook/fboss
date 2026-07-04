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

#include <iostream>
#include <string>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/table/gen-cpp2/bgp_table_summary_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {
using apache::thrift::util::enumNameSafe;
using facebook::fboss::utils::Table;
using neteng::fboss::bgp::thrift::TRibSummary;
using neteng::fboss::bgp_attr::TBgpAfi;

struct CmdShowBgpTableSummaryTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowBgpTableSummaryModel;
};

class CmdShowBgpTableSummary
    : public CmdHandler<CmdShowBgpTableSummary, CmdShowBgpTableSummaryTraits> {
 public:
  using RetType = CmdShowBgpTableSummaryTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

    TRibSummary v4;
    TRibSummary v6;
    client->sync_getRibSummary(v4, TBgpAfi::AFI_IPV4);
    client->sync_getRibSummary(v6, TBgpAfi::AFI_IPV6);

    cli::ShowBgpTableSummaryModel model;
    model.summaries() = {std::move(v4), std::move(v6)};
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& summary : model.summaries().value()) {
      out << "Address Family: " << enumNameSafe(summary.afi().value())
          << std::endl;
      out << "Total Prefixes: " << summary.total_prefixes().value()
          << std::endl;
      out << "  External (eBGP): " << summary.ebgp_prefixes().value()
          << "  Internal (iBGP): " << summary.ibgp_prefixes().value()
          << "  Confed-eBGP: " << summary.confed_ebgp_prefixes().value()
          << "  Local: " << summary.local_prefixes().value() << std::endl;

      Table table;
      table.setHeader({"Mask Length", "Number Of Prefixes"});
      // prefix_length_counts is an ordered map, so lengths print ascending.
      for (const auto& [maskLen, count] :
           summary.prefix_length_counts().value()) {
        table.addRow({"/" + std::to_string(maskLen), std::to_string(count)});
      }
      out << table << std::endl << std::endl;
    }
  }
};
} // namespace facebook::fboss
