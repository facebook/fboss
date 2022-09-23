// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/init/Init.h>
#include "fboss/qsfp_service/lib/QsfpClient.h"

using namespace facebook::fboss;
using namespace apache::thrift;

DEFINE_bool(phy_port_info, false, "Print PHY port SAI info, use with --port");
DEFINE_string(port, "", "Switch port");
DEFINE_bool(
    set_phy_loopback,
    false,
    "Set PHY port in loopback mode, use with --port --side");
DEFINE_bool(
    clear_phy_loopback,
    false,
    "Clear PHY port loopback mode, use with --port --side");
DEFINE_bool(
    set_admin_up,
    false,
    "Set PHY port Admin Up, use with --port --side");
DEFINE_bool(
    set_admin_down,
    false,
    "Set PHY port Admin Down, use with --port --side");
DEFINE_string(side, "line", "line side or system side");
DEFINE_bool(
    phy_cfg_check,
    false,
    "Check PHY config  in Hardware and report, use with --port");
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

void setPhyLoopback(QsfpServiceAsyncClient* qsfpHandler, bool setLoopback) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }
  if (FLAGS_side != "line" && FLAGS_side != "host") {
    printf("side should be either line or host\n");
    return;
  }

  phy::PortComponent component = (FLAGS_side == "line")
      ? phy::PortComponent::GB_LINE
      : phy::PortComponent::GB_SYSTEM;

  qsfpHandler->sync_setSaiPortLoopbackState(FLAGS_port, component, setLoopback);
  printf(
      "Port Loopback %s got %s\n",
      FLAGS_port.c_str(),
      (setLoopback ? "Set" : "Clear"));
}

void setPhyAdminState(QsfpServiceAsyncClient* qsfpHandler, bool adminUp) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }
  if (FLAGS_side != "line" && FLAGS_side != "host") {
    printf("side should be either line or host\n");
    return;
  }

  phy::PortComponent component = (FLAGS_side == "line")
      ? phy::PortComponent::GB_LINE
      : phy::PortComponent::GB_SYSTEM;

  qsfpHandler->sync_setSaiPortAdminState(FLAGS_port, component, adminUp);
  printf(
      "Port %s Admin state set to %s\n",
      FLAGS_port.c_str(),
      (adminUp ? "Up" : "Down"));
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

void phyConfigCheckHw(QsfpServiceAsyncClient* qsfpHandler) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }

  std::string output;
  qsfpHandler->sync_phyConfigCheckHw(output, FLAGS_port);
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
  if (FLAGS_set_phy_loopback) {
    setPhyLoopback(client.get(), true);
    return 0;
  }
  if (FLAGS_clear_phy_loopback) {
    setPhyLoopback(client.get(), false);
    return 0;
  }
  if (FLAGS_set_admin_up) {
    setPhyAdminState(client.get(), true);
    return 0;
  }
  if (FLAGS_set_admin_down) {
    setPhyAdminState(client.get(), false);
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
  if (FLAGS_phy_cfg_check) {
    phyConfigCheckHw(client.get());
    return 0;
  }
  return 0;
}
