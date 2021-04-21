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

#include "fboss/cli/fboss2/CmdClearArp.h"
#include "fboss/cli/fboss2/CmdClearNdp.h"
#include "fboss/cli/fboss2/CmdClientUtils.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdShowAcl.h"
#include "fboss/cli/fboss2/CmdShowArp.h"
#include "fboss/cli/fboss2/CmdShowLldp.h"
#include "fboss/cli/fboss2/CmdShowNdp.h"
#include "fboss/cli/fboss2/CmdUtils.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

// Avoid template linker error
// https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template void CmdHandler<CmdShowAcl, CmdShowAclTraits>::run();
template void CmdHandler<CmdShowArp, CmdShowArpTraits>::run();
template void CmdHandler<CmdShowLldp, CmdShowLldpTraits>::run();
template void CmdHandler<CmdShowNdp, CmdShowNdpTraits>::run();

template void CmdHandler<CmdClearArp, CmdClearArpTraits>::run();
template void CmdHandler<CmdClearNdp, CmdClearNdpTraits>::run();

template <typename CmdTypeT, typename CmdTypeTraits>
void CmdHandler<CmdTypeT, CmdTypeTraits>::run() {
  utils::setLogLevel(CmdGlobalOptions::getInstance()->getLogLevel());

  bool printHostName = (CmdGlobalOptions::getInstance()->getHosts().size() != 1);
  for (const auto& host : CmdGlobalOptions::getInstance()->getHosts()) {
    if (printHostName) {
      std::cout << std::endl << host << std::endl;
      std::cout << std::string(80, '-') << std::endl;
    }

    // Derive IP of the supplied host.
    auto hostIp = utils::getIPFromHost(host);
    XLOG(DBG2) << "host: " << host << " ip: " << hostIp.str();

    // Create desired client for the host.
    folly::EventBase evb;

    auto client = utils::createClient<ClientType>(hostIp.str(), evb);

    RetType result = impl().queryClient(client);
    impl().printOutput(result);
  }
}

} // namespace facebook::fboss
