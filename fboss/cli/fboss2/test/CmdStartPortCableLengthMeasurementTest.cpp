// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/start/port/cable_length_measurement/CmdStartPortCableLengthMeasurement.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

namespace {

state::SwitchState createSwitchState(
    const std::vector<std::pair<int16_t, std::string>>& ports) {
  state::SwitchState switchState;
  std::map<int16_t, state::PortFields> portMap;
  for (const auto& [portId, portName] : ports) {
    state::PortFields port;
    port.portId() = portId;
    port.portName() = portName;
    portMap[portId] = port;
  }
  switchState.portMaps()["id=0"] = portMap;
  return switchState;
}

} // namespace

class CmdStartPortCableLengthMeasurementTestFixture
    : public CmdHandlerTestBase {};

TEST_F(CmdStartPortCableLengthMeasurementTestFixture, queryClient) {
  setupMockedAgentServer();
  setupMockedHwAgentServer();

  EXPECT_CALL(getMockAgent(), getSwitchIndicesForInterfaces(_, _))
      .WillOnce([](std::map<int16_t, std::vector<std::string>>& switchToPorts,
                   std::unique_ptr<std::vector<std::string>> ports) {
        switchToPorts[0] = *ports;
      });

  auto switchState = createSwitchState({{1, "eth1/1/1"}, {2, "eth1/2/1"}});
  EXPECT_CALL(getMockHwAgent(), getProgrammedState(_))
      .WillOnce([&](state::SwitchState& state) { state = switchState; });
  EXPECT_CALL(getMockHwAgent(), triggerCableLengthMeasurement(_))
      .WillOnce([](std::unique_ptr<std::vector<int32_t>> ports) {
        EXPECT_THAT(*ports, ElementsAre(1, 2));
      });

  auto output = CmdStartPortCableLengthMeasurement().queryClient(
      localhost(), utils::PortList({"eth1/1/1", "eth1/2/1"}));
  EXPECT_EQ(
      output,
      "Triggered cable length measurement on port(s): eth1/1/1, eth1/2/1");
}

TEST_F(CmdStartPortCableLengthMeasurementTestFixture, printOutput) {
  std::stringstream ss;
  CmdStartPortCableLengthMeasurement().printOutput(
      "Triggered cable length measurement on port(s): eth1/1/1", ss);

  EXPECT_EQ(
      ss.str(), "Triggered cable length measurement on port(s): eth1/1/1\n");
}

} // namespace facebook::fboss
