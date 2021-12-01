// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/init/Init.h>
#include "fboss/util/CredoMacsecUtil.h"

using namespace facebook::fboss;

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
  folly::EventBase evb;
  std::unique_ptr<QsfpServiceAsyncClient> client =
      QsfpClient::createClient(&evb).getVia(&evb);
  CredoMacsecUtil macsec;

  if (FLAGS_phy_port_map) {
    macsec.printPhyPortMap(client.get());
    return 0;
  }
  if (FLAGS_add_sa_rx) {
    macsec.addSaRx(client.get());
    return 0;
  }
  if (FLAGS_add_sa_tx) {
    macsec.addSaTx(client.get());
    return 0;
  }
  if (FLAGS_delete_sa_rx) {
    macsec.deleteSaRx(client.get());
    return 0;
  }
  if (FLAGS_delete_sa_tx) {
    macsec.deleteSaTx(client.get());
    return 0;
  }
  if (FLAGS_delete_all_sc) {
    macsec.deleteAllSc(client.get());
    return 0;
  }
  if (FLAGS_phy_link_info) {
    macsec.printPhyLinkInfo(client.get());
    return 0;
  }
  if (FLAGS_phy_serdes_info) {
    macsec.printPhySerdesInfo(client.get());
    return 0;
  }
  if (FLAGS_get_all_sc_info) {
    macsec.getAllScInfo(client.get());
    return 0;
  }
  if (FLAGS_get_port_stats) {
    macsec.getPortStats(client.get());
    return 0;
  }
  if (FLAGS_get_flow_stats) {
    macsec.getFlowStats(client.get());
    return 0;
  }
  if (FLAGS_get_sa_stats) {
    macsec.getSaStats(client.get());
    return 0;
  }
  if (FLAGS_get_allport_stats) {
    macsec.getAllPortStats(client.get());
    return 0;
  }
  if (FLAGS_get_sdk_state) {
    macsec.getSdkState(client.get());
    return 0;
  }

  return 0;
}
