// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/ThreadLocal.h>
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include <folly/init/Init.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <folly/json.h>
#include <folly/FileUtil.h>
#include <folly/Random.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include "fboss/qsfp_service/lib/QsfpClient.h"

using namespace facebook::fboss;
using namespace facebook::fboss::mka;
using namespace apache::thrift;

DEFINE_bool(phy_port_map, false, "Print Phy Port map for this platform");

/*
 * printPhyPortMap()
 * Print the Phy port mapping information from the macsec handling process.
 * This function will make the thrift call to the qsfp_service (or phy_service)
 * process to get this phy port mapping information. ie:
 * PORT    SLOT    MDIO    PHYID   NAME
 *  1       2       1       0      eth4/2/1
 *  3       0       0       0      eth2/1/1
 *  5       3       4       0      eth5/5/1
 */
void printPhyPortMap(QsfpServiceAsyncClient* fbMacsecHandler) {
  MacsecPortPhyMap macsecPortPhyMap{};
  fbMacsecHandler->sync_macsecGetPhyPortInfo(macsecPortPhyMap);

  printf("Printing the Phy port info map:\n");
  printf("PORT    SLOT    MDIO    PHYID   NAME\n");
  for (auto it : *macsecPortPhyMap.macsecPortPhyMap_ref()) {
      int port = it.first;
      int slot = *it.second.slotId_ref();
      int mdio = *it.second.mdioId_ref();
      int phy = *it.second.phyId_ref();

      printf("%4d    %4d    %4d    %4d    eth%d/%d/1\n",  port, slot, mdio, phy, slot, mdio+1);
  }
}

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
  folly::EventBase evb;

  std::unique_ptr<QsfpServiceAsyncClient> client;
  client = QsfpClient::createClient(&evb).getVia(&evb);

  if (FLAGS_phy_port_map) {
    printPhyPortMap(client.get());
    return 0;
  }

  return 0;
}
