// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include <gmock/gmock.h>
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

std::vector<std::string> getPortList() {
  return {"eth1/1/1", "eth1/20/3", "eth1/32/4"};
}

std::vector<std::string> getPrbsComponent() {
  return {
      "asic",
      "xphy_system",
      "xphy_line",
      "transceiver_system",
      "transceiver_line"};
}

utils::PortList doTypingImplicitPortList() {
  return getPortList();
}

utils::PrbsComponent doTypingImplicitPrbsComponent() {
  return getPrbsComponent();
}

std::tuple<utils::PortList, utils::PrbsComponent> doTypingImplicitTuple() {
  return std::make_tuple(getPortList(), getPrbsComponent());
}

template <typename TypedArgs, typename UnTypedArgs>
TypedArgs doTyping(UnTypedArgs& unTypedArgs) {
  return unTypedArgs;
}

} // namespace

TEST(CmdArgsTest, doTypingImplicitSingle) {
  auto typedPortList = doTypingImplicitPortList();
  auto typedPrbsComponent = doTypingImplicitPrbsComponent();
  EXPECT_EQ(typeid(typedPortList), typeid(utils::PortList));
  EXPECT_EQ(typeid(typedPrbsComponent), typeid(utils::PrbsComponent));
  static_assert(std::is_same_v<utils::PortList, decltype(typedPortList)>);
  static_assert(
      std::is_same_v<utils::PrbsComponent, decltype(typedPrbsComponent)>);
}

TEST(CmdArgsTest, doTypingImplicitTuple) {
  auto typedArgs = doTypingImplicitTuple();
  static_assert(std::is_same_v<
                utils::PortList,
                std::tuple_element_t<0, decltype(typedArgs)>>);
  static_assert(std::is_same_v<
                utils::PrbsComponent,
                std::tuple_element_t<1, decltype(typedArgs)>>);
}

TEST(CmdArgsTest, doTypingTemplating) {
  using TypedArgsTest = std::tuple<utils::PortList, utils::PrbsComponent>;
  auto untypedArgs = std::make_tuple(getPortList(), getPrbsComponent());
  auto typedArgs = doTyping<TypedArgsTest>(untypedArgs);
  static_assert(std::is_same_v<
                utils::PortList,
                std::tuple_element_t<0, decltype(typedArgs)>>);
  static_assert(std::is_same_v<
                utils::PrbsComponent,
                std::tuple_element_t<1, decltype(typedArgs)>>);
}

TEST(CmdArgsTest, PortList) {
  // port name must conform to the required pattern 'moduleNum/port/subport'

  // test valid arguments
  ASSERT_NO_THROW(utils::PortList({"eth1/5/1"}));
  ASSERT_NO_THROW(utils::PortList({"eth1/5/1", "eth1/5/2"}));

  // test port data
  auto twoPorts = utils::PortList({"eth1/5/1", "eth1/5/2"});
  EXPECT_THAT(twoPorts.data(), ElementsAre("eth1/5/1", "eth1/5/2"));

  auto duplicatePorts = utils::PortList({"eth1/5/1", "eth1/5/1"});
  EXPECT_THAT(duplicatePorts.data(), ElementsAre("eth1/5/1"));

  auto multiPorts = utils::PortList({"eth1/5/1", "eth1/5/9", "eth1/5/1"});
  EXPECT_THAT(multiPorts.data(), ElementsAre("eth1/5/1", "eth1/5/9"));

  // test invalid arguments
  ASSERT_THROW(utils::PortList({"eth1"}), std::invalid_argument);
  ASSERT_THROW(utils::PortList({"eth1/5"}), std::invalid_argument);
  ASSERT_THROW(utils::PortList({"eth1/5/"}), std::invalid_argument);
  ASSERT_THROW(utils::PortList({"eth1//1"}), std::invalid_argument);
  ASSERT_THROW(utils::PortList({"eth/5/1"}), std::invalid_argument);
}

TEST(CmdArgsTest, FsdbPath) {
  // test valid arguments
  EXPECT_NO_THROW(utils::FsdbPath({"/agent/config"}));

  // test parsed results
  auto fsdbPath = utils::FsdbPath({"/agent/config"});
  EXPECT_THAT(fsdbPath.data(), ElementsAre("agent", "config"));

  fsdbPath = utils::FsdbPath({"/agent/switchState/portMap/eth2\\/1\\/1"});
  EXPECT_THAT(
      fsdbPath.data(),
      ElementsAre("agent", "switchState", "portMap", "eth2/1/1"));

  // empty is no throw
  EXPECT_NO_THROW(utils::FsdbPath({}));

  // multiple items is a throw
  EXPECT_THROW(utils::FsdbPath({"invalid", "args"}), std::runtime_error);
}

TEST(CmdArgsTest, HwObjectList) {
  // test valid arguments
  ASSERT_NO_THROW(utils::HwObjectList({"PORT"}));
  EXPECT_THAT(
      utils::HwObjectList({"PORT"}).data(), ElementsAre(HwObjectType::PORT));
  ASSERT_NO_THROW(utils::HwObjectList({"LAG", "MIRROR"}));
  EXPECT_THAT(
      utils::HwObjectList({"LAG", "MIRROR"}).data(),
      ElementsAre(HwObjectType::LAG, HwObjectType::MIRROR));

  // test invalid arguments
  ASSERT_THROW(utils::HwObjectList({"M"}), std::out_of_range);
  ASSERT_THROW(utils::HwObjectList({""}), std::out_of_range);
  ASSERT_THROW(
      utils::HwObjectList({"QUEUE", "VLAN", "AAA"}), std::out_of_range);
}

} // namespace facebook::fboss
