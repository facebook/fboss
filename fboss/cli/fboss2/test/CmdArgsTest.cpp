// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include "fboss/cli/fboss2/CmdHandler.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

using namespace ::testing;

namespace facebook::fboss {
namespace {

utils::PortList doTypingImplicitPortList() {
  std::vector<std::string> portList({"eth1/1/1", "eth1/20/3", "eth1/32/4"});
  return portList;
}

utils::PrbsComponent doTypingImplicitPrbsComponent() {
  std::vector<std::string> prbsComponent(
      {"asic",
       "xphy_system",
       "xphy_line",
       "transceiver_system",
       "transceiver_line"});
  return prbsComponent;
}

} // namespace

TEST(CmdArgsTest, doTypingImplicitSingle) {
  auto typedPortList = doTypingImplicitPortList();
  auto typedPrbsComponent = doTypingImplicitPrbsComponent();
  EXPECT_EQ(typeid(typedPortList), typeid(utils::PortList));
  EXPECT_EQ(typeid(typedPrbsComponent), typeid(utils::PrbsComponent));
}
} // namespace facebook::fboss
