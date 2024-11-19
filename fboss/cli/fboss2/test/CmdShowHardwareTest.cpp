// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/hardware/CmdShowHardware.h"
#include "fboss/cli/fboss2/commands/show/hardware/gen-cpp2/model_types.h"
#include "folly/json/dynamic.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
folly::dynamic createHardwareEntries() {
  folly::dynamic entries = folly::dynamic::object;
  entries["BGPDUptime"] = "1d 2h 3m 45s";
  entries["WedgeAgentUptime"] = "1d 2h 3m 45s";

  folly::dynamic pimInfo = folly::dynamic::object(
      "PIM2", {folly::dynamic::object("fpga_ver", "4.10")})(
      "PIM1", {folly::dynamic::object("fpga_ver", "4.10")});
  entries["PIMINFO"] = pimInfo;

  folly::dynamic pimpresent =
      folly::dynamic::object("pim2", "Present")("pim1", "Present");
  entries["PIMPRESENT"] = pimpresent;

  folly::dynamic pimserial =
      folly::dynamic::object("PIM2", "A123456")("PIM1", "A654321");

  entries["PIMSERIAL"] = pimserial;

  folly::dynamic seutil = folly::dynamic::object(
      "Local MAC", "A1:B2:C3:D4:E5:F6")("Product Serial Number", "Z123456");
  entries["SEUTIL"] = seutil;

  folly::dynamic fruid =
      folly::dynamic::object("Extended MAC Base", "F6:E5:D4:C3:B2:A1")(
          "Product Serial Number", "Z654321");
  entries["FRUID"] = fruid;

  return entries;
}

class CmdShowHardwareTestFixture : public testing::Test {
 public:
  folly::dynamic hardwareEntries;
  std::string product;

  void SetUp() override {
    hardwareEntries = createHardwareEntries();
    product = "MINIPACK-SMB";
  }
};

TEST_F(CmdShowHardwareTestFixture, createModel) {
  auto cmd = CmdShowHardware();
  auto model = cmd.createModel(hardwareEntries, product);
  auto entries = model.get_modules();

  EXPECT_EQ(entries.size(), 4);

  EXPECT_EQ(model.get_ctrlUptime(), "1d 2h 3m 45s");
  EXPECT_EQ(model.get_bgpdUptime(), "1d 2h 3m 45s");

  EXPECT_EQ(entries[0].get_moduleName(), "CHASSIS");
  EXPECT_EQ(entries[0].get_macAddress(), "F6:E5:D4:C3:B2:A1");
  EXPECT_EQ(entries[0].get_serialNumber(), "Z654321");

  EXPECT_EQ(entries[1].get_moduleName(), "PIM1");
  EXPECT_EQ(entries[1].get_serialNumber(), "A654321");
  EXPECT_EQ(entries[1].get_fpgaVersion(), "4.10");

  EXPECT_EQ(entries[2].get_moduleName(), "PIM2");
  EXPECT_EQ(entries[2].get_serialNumber(), "A123456");
  EXPECT_EQ(entries[2].get_fpgaVersion(), "4.10");

  EXPECT_EQ(entries[3].get_moduleName(), "SCM");
  EXPECT_EQ(entries[3].get_macAddress(), "A1:B2:C3:D4:E5:F6");
  EXPECT_EQ(entries[3].get_serialNumber(), "Z123456");
}

TEST_F(CmdShowHardwareTestFixture, printOutput) {
  auto cmd = CmdShowHardware();
  auto model = cmd.createModel(hardwareEntries, product);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " Module   Type     Serial   MAC                Status  FPGA Version \n"
      "---------------------------------------------------------------------------\n"
      " CHASSIS  CHASSIS  Z654321  F6:E5:D4:C3:B2:A1  OK      N/A          \n"
      " PIM1     PIM      A654321  N/A                OK      4.10         \n"
      " PIM2     PIM      A123456  N/A                OK      4.10         \n"
      " SCM      SCM      Z123456  A1:B2:C3:D4:E5:F6  OK      N/A          \n\n"
      "BGPD Uptime: 1d 2h 3m 45s\n"
      "Wedge Agent Uptime: 1d 2h 3m 45s\n";

  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
