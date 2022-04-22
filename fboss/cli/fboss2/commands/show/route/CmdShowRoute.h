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

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowRouteTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowRouteModel;
};

class CmdShowRoute : public CmdHandler<CmdShowRoute, CmdShowRouteTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedRoutes) {
    std::vector<facebook::fboss::RouteDetails> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getRouteTableDetails(entries);
    return createModel(entries, queriedRoutes);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& entry : model.get_routeEntries()) {
      out << fmt::format(
          "\nNetwork Address: {}/{}{}\n",
          entry.get_ip(),
          entry.get_prefixLength(),
          entry.get_isConnected() ? " (connected)" : "");

      for (const auto& clAndNxthops : entry.get_nextHopMulti()) {
        out << fmt::format(
            "  Nexthops from client {}\n", clAndNxthops.get_clientId());
        for (const auto& nextHop : clAndNxthops.get_nextHops()) {
          out << fmt::format("    {}\n", getNextHopInfoStr(nextHop));
        }
      }

      out << fmt::format("  Action: {}\n", entry.get_action());

      auto& nextHops = entry.get_nextHops();
      if (nextHops.size() > 0) {
        out << fmt::format("  Forwarding via:\n");
        for (const auto& nextHop : nextHops) {
          out << fmt::format("    {}\n", getNextHopInfoStr(nextHop));
        }
      } else {
        out << "  No Forwarding Info\n";
      }

      out << fmt::format("  Admin Distance: {}\n", entry.get_adminDistance());
      out << fmt::format("  Counter Id: {}\n", entry.get_counterID());
    }
  }

  RetType createModel(
      std::vector<facebook::fboss::RouteDetails> routeEntries,
      const ObjectArgType& queriedRoutes) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedRoutes.begin(), queriedRoutes.end());

    for (const auto& entry : routeEntries) {
      auto ipStr = getAddrStr(*entry.dest()->ip());
      auto ipPrefix =
          ipStr + "/" + std::to_string(*entry.dest()->prefixLength());
      if (queriedRoutes.size() == 0 || queriedSet.count(ipPrefix)) {
        cli::RouteEntry routeDetails;
        routeDetails.ip() = ipStr;
        routeDetails.prefixLength() = *entry.dest()->prefixLength();
        routeDetails.action() = *entry.action();
        routeDetails.isConnected() = *entry.isConnected();

        auto& nextHopMulti = entry.get_nextHopMulti();
        for (const auto& clAndNxthops : nextHopMulti) {
          cli::ClientAndNextHops clAndNxthopsCli;
          clAndNxthopsCli.clientId() = clAndNxthops.get_clientId();
          auto& nextHopAddrs = clAndNxthops.get_nextHopAddrs();
          auto& nextHops = clAndNxthops.get_nextHops();
          if (nextHopAddrs.size() > 0) {
            for (const auto& address : nextHopAddrs) {
              cli::NextHopInfo nextHopInfo;
              getNextHopInfo(address, nextHopInfo);
              clAndNxthopsCli.nextHops()->emplace_back(nextHopInfo);
            }
          } else if (nextHops.size() > 0) {
            for (const auto& nextHop : nextHops) {
              cli::NextHopInfo nextHopInfo;
              getNextHopInfo(nextHop, nextHopInfo);
              clAndNxthopsCli.nextHops()->emplace_back(nextHopInfo);
            }
          }
          routeDetails.nextHopMulti()->emplace_back(clAndNxthopsCli);
        }

        auto& fwdInfo = *entry.fwdInfo();
        auto& nextHops = entry.get_nextHops();
        if (nextHops.size() > 0) {
          for (const auto& nextHop : nextHops) {
            cli::NextHopInfo nextHopInfo;
            getNextHopInfo(nextHop, nextHopInfo);
            routeDetails.nextHops()->emplace_back(nextHopInfo);
          }
        } else if (fwdInfo.size() > 0) {
          for (const auto& ifAndIp : fwdInfo) {
            cli::NextHopInfo nextHopInfo;
            nextHopInfo.interfaceID() = ifAndIp.get_interfaceID();
            getNextHopInfo(ifAndIp.get_ip(), nextHopInfo);
            routeDetails.nextHops()->emplace_back(nextHopInfo);
          }
        }

        auto adminDistancePtr = entry.get_adminDistance();
        routeDetails.adminDistance() = adminDistancePtr == nullptr
            ? "None"
            : getAdminDistanceStr(*adminDistancePtr);

        auto counterIDPtr = entry.get_counterID();
        routeDetails.counterID() =
            counterIDPtr == nullptr ? "None" : *counterIDPtr;

        model.routeEntries()->push_back(routeDetails);
      }
    }
    return model;
  }

  std::string getAddrStr(network::thrift::BinaryAddress addr) {
    auto ip = *addr.addr();
    char ipBuff[INET6_ADDRSTRLEN];
    if (ip.size() == 16) {
      inet_ntop(
          AF_INET6,
          &((struct in_addr*)ip.c_str())->s_addr,
          ipBuff,
          INET6_ADDRSTRLEN);
    } else if (ip.size() == 4) {
      inet_ntop(
          AF_INET,
          &((struct in_addr*)ip.c_str())->s_addr,
          ipBuff,
          INET_ADDRSTRLEN);
    } else {
      return "invalid";
    }
    return std::string(ipBuff);
  }

  std::string getMplsActionCodeStr(MplsActionCode mplsActionCode) {
    switch (mplsActionCode) {
      case MplsActionCode::PUSH:
        return "PUSH";
      case MplsActionCode::SWAP:
        return "SWAP";
      case MplsActionCode::PHP:
        return "PHP";
      case MplsActionCode::POP_AND_LOOKUP:
        return "POP_AND_LOOKUP";
      case MplsActionCode::NOOP:
        return "NOOP";
    }
    throw std::runtime_error(
        "Unsupported MplsActionCode: " +
        std::to_string(static_cast<int>(mplsActionCode)));
  }

  std::string getMplsActionInfoStr(const cli::MplsActionInfo& mplsActionInfo) {
    auto action = mplsActionInfo.get_action();
    auto swapLabelPtr = mplsActionInfo.get_swapLabel();
    auto pushLabelsPtr = mplsActionInfo.get_pushLabels();
    std::string labels;

    if (action == "SWAP" && swapLabelPtr != nullptr) {
      labels = fmt::format(": {}", *swapLabelPtr);
    } else if (action == "PUSH" && pushLabelsPtr != nullptr) {
      auto stackStr = folly::join(",", *pushLabelsPtr);
      labels = fmt::format(": {{{}}}", stackStr);
    }
    return fmt::format(" MPLS -> {} {}", mplsActionInfo.get_action(), labels);
  }

  void getNextHopInfo(
      const network::thrift::BinaryAddress& addr,
      cli::NextHopInfo& nextHopInfo) {
    nextHopInfo.addr() = getAddrStr(addr);
    auto ifNamePtr = addr.get_ifName();
    if (ifNamePtr != nullptr) {
      nextHopInfo.ifName() = *ifNamePtr;
    }
  }

  void getNextHopInfo(
      const NextHopThrift& nextHop,
      cli::NextHopInfo& nextHopInfo) {
    getNextHopInfo(nextHop.get_address(), nextHopInfo);
    nextHopInfo.weight() = nextHop.get_weight();

    auto mplsActionPtr = nextHop.get_mplsAction();
    if (mplsActionPtr != nullptr) {
      cli::MplsActionInfo mplsActionInfo;
      mplsActionInfo.action() =
          getMplsActionCodeStr(mplsActionPtr->get_action());
      auto swapLabelPtr = mplsActionPtr->get_swapLabel();
      auto pushLabelsPtr = mplsActionPtr->get_pushLabels();
      if (swapLabelPtr != nullptr) {
        mplsActionInfo.swapLabel() = *swapLabelPtr;
      }
      if (pushLabelsPtr != nullptr) {
        mplsActionInfo.pushLabels() = *pushLabelsPtr;
      }
      nextHopInfo.mplsAction() = mplsActionInfo;
    }
  }

  std::string getNextHopInfoStr(const cli::NextHopInfo& nextHopInfo) {
    auto ifNamePtr = nextHopInfo.get_ifName();
    std::string viaStr;
    if (ifNamePtr != nullptr) {
      viaStr = fmt::format(" dev {}", *ifNamePtr);
    }
    std::string labelStr;
    auto mplsActionInfoPtr = nextHopInfo.get_mplsAction();
    if (mplsActionInfoPtr != nullptr) {
      labelStr = getMplsActionInfoStr(*mplsActionInfoPtr);
    }
    auto interfaceIDPtr = nextHopInfo.get_interfaceID();
    std::string interfaceIDStr;
    if (interfaceIDPtr != nullptr) {
      interfaceIDStr = fmt::format("(i/f {}) ", *interfaceIDPtr);
    }
    std::string weightStr;
    if (nextHopInfo.get_weight()) {
      weightStr = fmt::format(" weight {}", nextHopInfo.get_weight());
    }
    auto ret = fmt::format(
        "{}{}{}{}{}",
        interfaceIDStr,
        nextHopInfo.get_addr(),
        viaStr,
        weightStr,
        labelStr);
    return ret;
  }

  std::string getAdminDistanceStr(AdminDistance adminDistance) {
    switch (adminDistance) {
      case AdminDistance::DIRECTLY_CONNECTED:
        return "DIRECTLY_CONNECTED";
      case AdminDistance::STATIC_ROUTE:
        return "STATIC_ROUTE";
      case AdminDistance::OPENR:
        return "OPENR";
      case AdminDistance::EBGP:
        return "EBGP";
      case AdminDistance::IBGP:
        return "IBGP";
      case AdminDistance::MAX_ADMIN_DISTANCE:
        return "MAX_ADMIN_DISTANCE";
    }
    throw std::runtime_error(
        "Unsupported AdminDistance: " +
        std::to_string(static_cast<int>(adminDistance)));
  }
};

} // namespace facebook::fboss
