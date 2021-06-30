/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdHandler.h"

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/clear/CmdClearArp.h"
#include "fboss/cli/fboss2/commands/clear/CmdClearNdp.h"
#include "fboss/cli/fboss2/commands/show/acl/CmdShowAcl.h"
#include "fboss/cli/fboss2/commands/show/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/lldp/CmdShowLldp.h"
#include "fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/commands/show/CmdShowPortQueue.h"
#include "fboss/cli/fboss2/commands/show/arp/CmdShowArp.h"
#include "fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "folly/futures/Future.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include <future>
#include <iostream>

template <typename CmdTypeT>
void printTabular(
    CmdTypeT& cmd,
    std::vector<std::future<
        std::tuple<std::string, typename CmdTypeT::RetType, std::string>>>&
        results,
    std::ostream& out,
    std::ostream& err) {
  for (auto& result : results) {
    auto [host, data, errStr] = result.get();
    if (results.size() != 1) {
      out << host << "::" << std::endl << std::string(80, '=') << std::endl;
    }

    if (errStr.empty()) {
      cmd.printOutput(data);
    } else {
      err << errStr << std::endl << std::endl;
    }
  }
}

template <typename CmdTypeT>
void printJson(
    const CmdTypeT& /* cmd */,
    std::vector<std::future<
        std::tuple<std::string, typename CmdTypeT::RetType, std::string>>>&
        results,
    std::ostream& out,
    std::ostream& err) {
  std::map<std::string, typename CmdTypeT::RetType> hostResults;
  for (auto& result : results) {
    auto [host, data, errStr] = result.get();
    if (errStr.empty()) {
      hostResults[host] = data;
    } else {
      err << host << "::" << std::endl << std::string(80, '=') << std::endl;
      err << errStr << std::endl << std::endl;
    }
  }

  out << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
             hostResults)
      << std::endl;
}

namespace facebook::fboss {

// Avoid template linker error
// https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template void CmdHandler<CmdShowAcl, CmdShowAclTraits>::run();
template void CmdHandler<CmdShowArp, CmdShowArpTraits>::run();
template void CmdHandler<CmdShowLldp, CmdShowLldpTraits>::run();
template void CmdHandler<CmdShowNdp, CmdShowNdpTraits>::run();
template void CmdHandler<CmdShowPort, CmdShowPortTraits>::run();
template void CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits>::run();
template void CmdHandler<CmdShowInterface, CmdShowInterfaceTraits>::run();
template void CmdHandler<CmdShowTransceiver, CmdShowTransceiverTraits>::run();

template void CmdHandler<CmdClearArp, CmdClearArpTraits>::run();
template void CmdHandler<CmdClearNdp, CmdClearNdpTraits>::run();

static bool hasRun = false;

template <typename CmdTypeT, typename CmdTypeTraits>
void CmdHandler<CmdTypeT, CmdTypeTraits>::run() {
  // Parsing library invokes every chained command handler, but we only need
  // the 'leaf' command handler to be invoked. Thus, after the first (leaf)
  // command handler is invoked, simply return.
  // TODO: explore if the parsing library provides a better way to implement
  // this.
  if (hasRun) {
    return;
  }

  hasRun = true;

  utils::setLogLevel(CmdGlobalOptions::getInstance()->getLogLevel());
  utils::logUsage(folly::demangle(typeid(this)).toStdString());

  if (!CmdGlobalOptions::getInstance()->isValid()) {
    exit(1);
  }

  std::vector<std::string> hosts;
  if (!CmdGlobalOptions::getInstance()->getHosts().empty()) {
    hosts = CmdGlobalOptions::getInstance()->getHosts();
  } else if (!CmdGlobalOptions::getInstance()->getSmc().empty()) {
    hosts = utils::getHostsInSmcTier(CmdGlobalOptions::getInstance()->getSmc());
  } else if (!CmdGlobalOptions::getInstance()->getFile().empty()) {
    hosts = utils::getHostsFromFile(CmdGlobalOptions::getInstance()->getFile());
  } else { // if host is not specified, default to localhost
    hosts = {"localhost"};
  }

  std::vector<std::future<std::tuple<std::string, RetType, std::string>>>
      futureList;
  for (const auto& host : hosts) {
    futureList.push_back(std::async(
        std::launch::async,
        &CmdHandler::asyncHandler,
        this,
        host /*, inArgs*/));
  }

  if (CmdGlobalOptions::getInstance()->getFmt() == "tabular") {
    printTabular(impl(), futureList, std::cout, std::cerr);
  } else if (CmdGlobalOptions::getInstance()->getFmt() == "JSON") {
    printJson(impl(), futureList, std::cout, std::cerr);
  }
}

} // namespace facebook::fboss
