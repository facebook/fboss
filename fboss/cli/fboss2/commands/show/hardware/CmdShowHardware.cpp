/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/hardware/CmdShowHardware.h"

#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/cli/fboss2/utils/clients/BmcClient.h"
#include "folly/json/json.h"
#ifndef IS_OSS
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#endif

#include <boost/algorithm/string.hpp>

namespace facebook::fboss {

CmdShowHardware::RetType CmdShowHardware::queryClient(
    const HostInfo& hostInfo) {
  folly::dynamic entries = folly::dynamic::object;
  facebook::fboss::ProductInfo product_info;

  // Initialize the clients we will be using
  // This command makes multiple calls to the BMC.  Due to the slow/single
  // threaded Nature of the BMC, this command has a rather slow runtime.
  // Unfortunately this is expected
  auto bmc = utils::createClient<BmcClient>(hostInfo);
  auto wedgeClient =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
#ifndef IS_OSS
  auto bgpClient = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
#endif

  // Product info helps us make some decisions when there need to be per
  // platform treatment of certain outputs
  wedgeClient->sync_getProductInfo(product_info);
  std::string product = product_info.product().value();

#ifndef IS_OSS
  // AliveSince calls use the inherited FB303 counters
  std::int64_t bgpAliveSince = bgpClient->sync_aliveSince();
  entries["BGPDUptime"] = utils::getPrettyElapsedTime(bgpAliveSince);
#endif

  std::int64_t wedgeAliveSince = wedgeClient->sync_aliveSince();
  entries["WedgeAgentUptime"] = utils::getPrettyElapsedTime(wedgeAliveSince);

  std::map<std::string, std::string> endpoints = bmc->get_endpoints();
  // Not all platforms will have all of these endpoints.  Any null results
  // won't be added to the data structure.  The structure is a simple map of
  // the endpoint name to the JSON returned from the HTTP call
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
CmdShowHardware::RetType CmdShowHardware::createModel(
    folly::dynamic data,
    const std::string& product) {
  RetType ret;

  if (data.find("WedgeAgentUptime") != data.items().end()) {
    ret.ctrlUptime() = data["WedgeAgentUptime"].asString();
  } else {
    ret.ctrlUptime() = "DOWN";
  }
#ifndef IS_OSS
  if (data.find("BGPDUptime") != data.items().end()) {
    ret.bgpdUptime() = data["BGPDUptime"].asString();
  } else {
    ret.bgpdUptime() = "CRASHED";
  }
#endif
  // Process PIMs for platforms that have PIMS
  if (auto pimInfo = data.find("PIMINFO"); pimInfo != data.items().end()) {
    for (const auto& [pim, pimdata] : pimInfo->second.items()) {
      cli::HWModule hwmod;
      hwmod.moduleName() = pim.asString();
      // TODO: This is a quick and dirty hack to restore functionality until
      // T184560911 is resolved.
      if (product == "MINIPACK2") {
        if (pim.asString() == "PIM2") {
          hwmod.moduleName() = "pim1";
        } else if (pim.asString() == "PIM3") {
          hwmod.moduleName() = "pim2";
        } else if (pim.asString() == "PIM4") {
          hwmod.moduleName() = "pim3";
        } else if (pim.asString() == "PIM5") {
          hwmod.moduleName() = "pim4";
        } else if (pim.asString() == "PIM6") {
          hwmod.moduleName() = "pim5";
        } else if (pim.asString() == "PIM7") {
          hwmod.moduleName() = "pim6";
        } else if (pim.asString() == "PIM8") {
          hwmod.moduleName() = "pim7";
        } else if (pim.asString() == "PIM9") {
          hwmod.moduleName() = "pim8";
        }
      }
      hwmod.moduleType() = "PIM";
      hwmod.serialNumber() = data["PIMSERIAL"][pim].asString();
      std::string lower_pim =
          boost::algorithm::to_lower_copy(hwmod.moduleName().value());
      std::string pim_status;
      // Unfortunately, BMC does not adhere to a single standard so detecting
      // PIM presence is on a per platform basis.  So far we have observerd
      // that if a chassis has PIMs it will have some indication of if they
      // are present Have to look for both endpoints to determine presence.
      if (data.find("PIMPRESENT") != data.items().end()) {
        pim_status = data["PIMPRESENT"][lower_pim].asString();
      } else if (data.find("PRESENCE") != data.items().end()) {
        // Minipack2 does not use 'PIMPRESENT' Endpoint.  Instead it uses a
        // binary value in 'PRESENCE' endpoint which is a list of JSON objects
        // Would be nice to have BMC team standardize this
        for (const auto& element : data["PRESENCE"]) {
          for (const auto& [k, v] : element.items()) {
            if (boost::algorithm::trim_copy(k.asString()) == lower_pim) {
              pim_status = v == "1" ? "Present" : "NotPresent";
            }
          }
        }
      }
      hwmod.modStatus() = pim_status == "Present" ? "OK" : "Fail";
      hwmod.fpgaVersion() = pimdata["fpga_ver"].asString();
      ret.modules()->push_back(hwmod);
    }
  }
  // Wedge swtiches will only have a single line item so handle those now.
  if (boost::algorithm::starts_with(product, "WEDGE")) {
    if (data.find("FRUID") != data.items().end()) {
      cli::HWModule chassis;
      chassis.moduleName() = "CHASSIS";
      chassis.moduleType() = data["FRUID"]["Product Name"].asString();
      chassis.serialNumber() =
          data["FRUID"]["Product Serial Number"].asString();
      chassis.macAddress() = data["FRUID"]["Local MAC"].asString();
      chassis.modStatus() = "OK";
      ret.modules()->push_back(chassis);
    }
  }
  // Handle the rest of the FRU's for minipack/glacier chasiss
  else {
    if (data.find("FRUID") != data.items().end()) {
      cli::HWModule chassis;
      chassis.moduleName() = "CHASSIS";
      chassis.moduleType() = "CHASSIS";
      chassis.serialNumber() =
          data["FRUID"]["Product Serial Number"].asString();
      chassis.macAddress() = data["FRUID"]["Extended MAC Base"].asString();
      chassis.modStatus() = "OK";
      ret.modules()->push_back(chassis);
    }
    // Again we have deviation of endpoints amongst platforms so we have to
    // read from different locations to populate our object
    std::string fab_endpoint;
    if (data.find("SEUTIL") != data.items().end()) {
      fab_endpoint = "SEUTIL";
    } else if (data.find("SEUTIL_MP2") != data.items().end()) {
      fab_endpoint = "SEUTIL_MP2";
    }
    if (fab_endpoint != "") {
      cli::HWModule fab;
      fab.moduleName() = "SCM";
      fab.moduleType() = "SCM";
      fab.serialNumber() =
          data[fab_endpoint]["Product Serial Number"].asString();
      fab.macAddress() = data[fab_endpoint]["Local MAC"].asString();
      fab.modStatus() = "OK";
      ret.modules()->push_back(fab);
    }
  }
  // Sort our list of modules before outputing to table
  std::sort(
      ret.modules()->begin(),
      ret.modules()->end(),
      [](cli::HWModule& a, cli::HWModule& b) {
        return a.moduleName().value() < b.moduleName().value();
      });
  return ret;
}

/*
  Output to human readable format
*/
void CmdShowHardware::printOutput(const RetType& model, std::ostream& out) {
  // Recommended to use the Table library if suitable
  Table table;
  table.setHeader(
      {"Module", "Type", "Serial", "MAC", "Status", "FPGA Version"});

  for (auto const& data : model.modules().value()) {
    const auto& moduleName = data.moduleName().value();
    const auto& moduleType = data.moduleType().value();
    const auto& serialNumber = data.serialNumber().value();
    const auto& macAddress = data.macAddress().value();
    const auto& modStatus = data.modStatus().value();
    const auto& fpgaVersion = data.fpgaVersion().value();
    table.addRow({
        !moduleName.empty() ? moduleName : "N/A",
        !moduleType.empty() ? moduleType : "N/A",
        !serialNumber.empty() ? Table::StyledCell(serialNumber)
                              : Table::StyledCell("N/A", Table::Style::ERROR),
        !macAddress.empty() ? macAddress : "N/A",
        modStatus == "OK" ? Table::StyledCell("OK", Table::Style::GOOD)
                          : Table::StyledCell("N/A", Table::Style::ERROR),
        !fpgaVersion.empty() ? fpgaVersion : "N/A",
    });
  }

  out << table << std::endl;
#ifndef IS_OSS
  out << "BGPD Uptime: " + model.bgpdUptime().value() << std::endl;
#endif
  out << "Wedge Agent Uptime: " + model.ctrlUptime().value() << std::endl;
}

} // namespace facebook::fboss
