// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/init/Init.h>
#include "fboss/qsfp_service/lib/QsfpClient.h"

using namespace facebook::fboss;
using namespace apache::thrift;

DEFINE_bool(phy_port_info, false, "Print PHY port SAI info, use with --port");
DEFINE_string(port, "", "Switch port");
DEFINE_bool(
    read_reg,
    false,
    "Read PHY register, use with --port --mdio --dev --offset");
DEFINE_bool(
    write_reg,
    false,
    "Write PHY register, use with --port --mdio --dev --offset --val");
DEFINE_int32(mdio, -1, "PHY Mdio address");
DEFINE_int32(dev, -1, "Clause45 Device Id for register access");
DEFINE_int32(offset, -1, "Register offset");
DEFINE_int32(val, -1, "Value to be written");

void printPhyPortInfo(QsfpServiceAsyncClient* qsfpHandler) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }
  std::string output;
  qsfpHandler->sync_getSaiPortInfo(output, FLAGS_port);

  printf("%s\n", output.c_str());
}

void saiPhyRegisterAccess(QsfpServiceAsyncClient* qsfpHandler, bool opRead) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }
  if (FLAGS_mdio == -1 || FLAGS_dev == -1 || FLAGS_offset == -1) {
    printf("MDIO address, Device Id and Register offset is required\n");
    return;
  }
  if (!opRead && FLAGS_val == -1) {
    printf("Register value is required\n");
    return;
  }

  std::string output;
  qsfpHandler->sync_saiPhyRegisterAccess(
      output,
      FLAGS_port,
      opRead,
      FLAGS_mdio,
      FLAGS_dev,
      FLAGS_offset,
      FLAGS_val);

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
  if (FLAGS_read_reg) {
    saiPhyRegisterAccess(client.get(), true);
    return 0;
  }
  if (FLAGS_write_reg) {
    saiPhyRegisterAccess(client.get(), false);
    return 0;
  }
  return 0;
}
