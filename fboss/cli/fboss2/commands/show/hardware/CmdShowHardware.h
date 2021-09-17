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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/hardware/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/clients/BmcClient.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/json.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cstdint>


namespace facebook::fboss {

using utils::Table;

/*
 Define the traits of this command. This will include the inputs and output
 types
*/
struct CmdShowHardwareTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowHardwareModel;
};


class CmdShowHardware : public CmdHandler<CmdShowHardware, CmdShowHardwareTraits> {
 public:
  using ObjectArgType = CmdShowHardwareTraits::ObjectArgType;
  using RetType = CmdShowHardwareTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    folly::dynamic entries = folly::dynamic::object;
    facebook::fboss::ProductInfo product_info;

    // Initialize the clients we will be using
    // This command makes multiple calls to the BMC.  Due to the slow/single threaded
    // Nature of the BMC, this command has a rather slow runtime.  Unfortunately this
    // is expected
    auto bmc = utils::createClient<BmcClient>(hostInfo);
    auto wedgeClient = utils::createClient<FbossCtrlAsyncClient>(hostInfo);
    auto bgpClient = utils::createClient<
      facebook::neteng::fboss::bgp::thrift::TBgpServiceAsyncClient>(hostInfo);

    // Product info helps us make some decisions when there need to be per platform
    // treatment of certain outputs
    wedgeClient->sync_getProductInfo(product_info);
    std::string product = product_info.get_product();

    // AliveSince calls use the inherited FB303 counters
    std::int64_t bgpAliveSince = bgpClient->sync_aliveSince();
    entries["BGPDUptime"] = utils::getPrettyElapsedTime(bgpAliveSince);

    std::int64_t wedgeAliveSince = wedgeClient->sync_aliveSince();
    entries["WedgeAgentUptime"] = utils::getPrettyElapsedTime(wedgeAliveSince);

    std::map<std::string, std::string> endpoints = bmc->get_endpoints();
    // Not all platforms will have all of these endpoints.  Any null results won't
    // be added to the data structure.  The structure is a simple map of the
    // endpoint name to the JSON returned from the HTTP call
    for (const auto& endpoint : endpoints) {
      auto bmc_result = bmc->fetchRaw(endpoint.second);
      if (!bmc_result.isNull()) {
        entries[endpoint.first] = bmc_result["Information"];
      }
    }

    return createModel(entries, product);
  }

  /*
    Run data normalization to linked thrift object
  */
  RetType createModel(folly::dynamic data, std::string product) {
    RetType ret;

    if (data.find("WedgeAgentUptime") != data.items().end()) {
      ret.ctrlUptime_ref() = data["WedgeAgentUptime"].asString();
    }
    else {
      ret.ctrlUptime_ref() = "DOWN";
    }
    if (data.find("BGPDUptime") != data.items().end()) {
      ret.bgpdUptime_ref() = data["BGPDUptime"].asString();
    }
    else {
      ret.bgpdUptime_ref() = "CRASHED";
    }
    //Process PIMs for platforms that have PIMS
    if (auto pimInfo = data.find("PIMINFO"); pimInfo != data.items().end()) {
      for (const auto& [pim, pimdata] : pimInfo->second.items()) {
        cli::HWModule hwmod;
        hwmod.moduleName_ref() = pim.asString();
        hwmod.moduleType_ref() = "PIM";
        hwmod.serialNumber_ref() = data["PIMSERIAL"][pim].asString();
        std::string lower_pim = boost::algorithm::to_lower_copy(pim.asString());
        std::string pim_status = "";
        // Unfortunately, BMC does not adhere to a single standard so detecting
        // PIM presence is on a per platform basis.  So far we have observerd that
        // if a chassis has PIMs it will have some indication of if they are present
        // Have to look for both endpoints to determine presence.
        if (data.find("PIMPRESENT") != data.items().end()) {
          pim_status = data["PIMPRESENT"][lower_pim].asString();
        }
        else if (data.find("PRESENCE") != data.items().end()) {
          // Minipack2 does not use 'PIMPRESENT' Endpoint.  Instead it uses a
          // binary value in 'PRESENCE' endpoint which is a list of JSON objects
          // Would be nice to have BMC team standardize this
          for (const auto& element : data["PRESENCE"]) {
            for (const auto& [k, v] : element.items()) {
              if (boost::algorithm::trim_copy(k.asString()) == lower_pim) {
                pim_status = v == "1" ? "Present":"NotPresent";
              }
            }
          }
        }
        hwmod.modStatus_ref() = pim_status == "Present" ? "OK":"Fail";
        hwmod.fpgaVersion_ref() = pimdata["fpga_ver"].asString();
        ret.modules_ref()->push_back(hwmod);
      }
    }
    // Wedge swtiches will only have a single line item so handle those now.
    if (boost::algorithm::starts_with(product, "WEDGE")) {
      if (data.find("FRUID") != data.items().end()) {
        cli::HWModule chassis;
        chassis.moduleName_ref() = "CHASSIS";
        chassis.moduleType_ref() = data["FRUID"]["Product Name"].asString();
        chassis.serialNumber_ref() = data["FRUID"]["Product Serial Number"].asString();
        chassis.macAddress_ref() = data["FRUID"]["Local MAC"].asString();
        chassis.modStatus_ref() = "OK";
        ret.modules_ref()->push_back(chassis);
      }
    }
    // Handle the rest of the FRU's for minipack/glacier chasiss
    else {
      if (data.find("FRUID") != data.items().end()) {
        cli::HWModule chassis;
        chassis.moduleName_ref() = "CHASSIS";
        chassis.moduleType_ref() = "CHASSIS";
        chassis.serialNumber_ref() = data["FRUID"]["Product Serial Number"].asString();
        chassis.macAddress_ref() = data["FRUID"]["Extended MAC Base"].asString();
        chassis.modStatus_ref() = "OK";
        ret.modules_ref()->push_back(chassis);
      }
      // Again we have deviation of endpoints amongst platforms so we have to read
      // from different locations to populate our object
      std::string fab_endpoint = "";
      if (data.find("SEUTIL") != data.items().end()) {
        fab_endpoint = "SEUTIL";
      }
      else if (data.find("SEUTIL_MP2") != data.items().end()) {
        fab_endpoint = "SEUTIL_MP2";
      }
      if (fab_endpoint != "") {
        cli::HWModule fab;
        fab.moduleName_ref() = "SCM";
        fab.moduleType_ref() = "SCM";
        fab.serialNumber_ref() = data[fab_endpoint]["Product Serial Number"].asString();
        fab.macAddress_ref() = data[fab_endpoint]["Local MAC"].asString();
        fab.modStatus_ref() = "OK";
        ret.modules_ref()->push_back(fab);
      }
    }
    // Sort our list of modules before outputing to table
    std::sort(
      ret.modules_ref()->begin(),
      ret.modules_ref()->end(),
      [](cli::HWModule& a, cli::HWModule& b) {
        return a.get_moduleName() < b.get_moduleName();
      }
    );
    return ret;
  }

  /*
    Output to human readable format
  */
  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    // Recommended to use the Table library if suitable
    Table table;
    table.setHeader({
      "Module", "Type", "Serial", "MAC", "Status", "FPGA Version"});

    for (auto const& data : model.get_modules()) {
      table.addRow({
        data.get_moduleName() != "" ? data.get_moduleName() : "N/A",
        data.get_moduleType() != "" ? data.get_moduleType() : "N/A",
        data.get_serialNumber() != ""
          ? data.get_serialNumber() : Table::StyledCell("N/A", Table::Style::ERROR),
        data.get_macAddress() != "" ? data.get_macAddress() : "N/A",
        data.get_modStatus() == "OK"
          ?  Table::StyledCell("OK", Table::Style::GOOD) : Table::StyledCell("N/A", Table::Style::ERROR),
        data.get_fpgaVersion() != "" ? data.get_fpgaVersion() : "N/A",
      });
    }

    out << table << std::endl;
    out << "BGPD Uptime: " + model.get_ctrlUptime() << std::endl;
    out << "Wedge Agent Uptime: " + model.get_bgpdUptime() << std::endl;
  }
};

} // namespace facebook::fboss
