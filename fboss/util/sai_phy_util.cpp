// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/init/Init.h>
#include "fboss/qsfp_service/lib/QsfpClient.h"

using namespace facebook::fboss;
using namespace apache::thrift;

DEFINE_bool(phy_port_info, false, "Print PHY port SAI info, use with --port");
DEFINE_string(port, "", "Switch port");

void printPhyPortInfo(QsfpServiceAsyncClient* qsfpHandler) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }
  std::string output;
  qsfpHandler->sync_getSaiPortInfo(output, FLAGS_port);

  printf("%s\n", output.c_str());
}

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
  folly::EventBase evb;
  std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> client =
      QsfpClient::createClient(&evb).getVia(&evb);

  if (FLAGS_phy_port_info) {
    printPhyPortInfo(client.get());
    return 0;
  }

  return 0;
}
