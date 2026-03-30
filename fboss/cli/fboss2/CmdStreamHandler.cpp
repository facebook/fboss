/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/*
 * The inclusion of this .cpp by Impl files is odd but deliberate.
 *
 * Template linker error for run() can be avoided by:
 *  - defining run() in the header, but every subcommand must include the
 *    header so that can result in code bloat.
 *  - alternative is to add template lines such as in CmdStreamHandlerImpl.cpp.
 *
 * This technique is recommended in isocpp FAQ:
 * https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
 */

#include "fboss/cli/fboss2/CmdStreamHandler.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>

template <typename UnitT>
void printJson(const UnitT& val, std::ostream& out) {
  out << apache::thrift::SimpleJSONSerializer::serialize<std::string>(val)
      << std::endl;
}

template <typename CmdTypeT>
folly::coro::Task<void> printStream(
    CmdTypeT& cmd,
    typename CmdTypeT::StreamRetType& data,
    std::ostream& out,
    std::ostream& err) {
  try {
    if (facebook::fboss::CmdGlobalOptions::getInstance()->getFmt().isJson()) {
      while (auto val = co_await data.next()) {
        printJson(*val, out);
      }
    } else {
      while (auto val = co_await data.next()) {
        cmd.printOutput(*val, out);
      }
    }
  } catch (const std::exception& e) {
    // server sent an exception,
    // stopping the stream for some reason
    // eg: the service is restarting
    err << "Server Error: " << e.what() << std::endl;
    exit(2);
  }
  co_return;
}

namespace facebook::fboss {

template <typename CmdTypeT, typename CmdTypeTraits>
void CmdStreamHandler<CmdTypeT, CmdTypeTraits>::run(std::ostream& out) {
  auto hosts = getHosts();

  if (hosts.size() != 1) {
    XLOG(DBG2) << "Only support one host for now" << std::endl;
    exit(1);
  }

  const auto& host = hosts[0];
  auto hostInfo = HostInfo(host);
  XLOG(DBG2) << "host: " << host << " ip: " << hostInfo.getIpStr();

  StreamRetType generator;

  try {
    generator = queryClientHelper(hostInfo);
  } catch (std::exception const& err) {
    // Connection error should be thrown right away
    // Due to the use of AsyncGenerator, the error will
    // be delayed
    std::string errStr =
        folly::to<std::string>("Thrift call failed: '", err.what(), "'");
    std::cerr << errStr << std::endl;
    exit(2);
  }

  folly::coro::blockingWait(printStream(impl(), generator, out, std::cerr));
}

} // namespace facebook::fboss
