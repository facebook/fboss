/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <boost/filesystem/operations.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigInterfaceTestFixture()
      : CmdConfigTestBase(
            "fboss_interface_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000,
        "description": "original description of eth1/1/1"
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000,
        "description": "original description of eth1/2/1"
      }
    ],
    "vlanPorts": [
      {
        "vlanID": 1,
        "logicalPort": 1
      },
      {
        "vlanID": 2,
        "logicalPort": 2
      }
    ],
    "interfaces": [
      {
        "intfID": 1,
        "routerID": 0,
        "vlanID": 1,
        "name": "eth1/1/1",
        "mtu": 1500
      },
      {
        "intfID": 2,
        "routerID": 0,
        "vlanID": 2,
        "name": "eth1/2/1",
        "mtu": 1500
      }
    ]
  }
})") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupSpeedValidationMocks();
  }

  void setupSpeedValidationMocks() {
    // Create a mock PlatformMapping with common speeds supported
    cfg::PlatformMapping platformMapping;

    // Add platform supported profiles for common speeds
    // 10G
    cfg::PlatformPortProfileConfigEntry profile10G;
    profile10G.factor()->profileID() =
        cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER;
    profile10G.profile()->speed() = cfg::PortSpeed::XG;
    platformMapping.platformSupportedProfiles()->push_back(profile10G);

    // 25G
    cfg::PlatformPortProfileConfigEntry profile25G;
    profile25G.factor()->profileID() =
        cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER;
    profile25G.profile()->speed() = cfg::PortSpeed::TWENTYFIVEG;
    platformMapping.platformSupportedProfiles()->push_back(profile25G);

    // 40G
    cfg::PlatformPortProfileConfigEntry profile40G;
    profile40G.factor()->profileID() =
        cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC;
    profile40G.profile()->speed() = cfg::PortSpeed::FORTYG;
    platformMapping.platformSupportedProfiles()->push_back(profile40G);

    // 50G
    cfg::PlatformPortProfileConfigEntry profile50G;
    profile50G.factor()->profileID() =
        cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER;
    profile50G.profile()->speed() = cfg::PortSpeed::FIFTYG;
    platformMapping.platformSupportedProfiles()->push_back(profile50G);

    // 100G
    cfg::PlatformPortProfileConfigEntry profile100G;
    profile100G.factor()->profileID() =
        cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91;
    profile100G.profile()->speed() = cfg::PortSpeed::HUNDREDG;
    platformMapping.platformSupportedProfiles()->push_back(profile100G);

    // 200G
    cfg::PlatformPortProfileConfigEntry profile200G;
    profile200G.factor()->profileID() =
        cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N;
    profile200G.profile()->speed() = cfg::PortSpeed::TWOHUNDREDG;
    platformMapping.platformSupportedProfiles()->push_back(profile200G);

    // 400G
    cfg::PlatformPortProfileConfigEntry profile400G;
    profile400G.factor()->profileID() =
        cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N;
    profile400G.profile()->speed() = cfg::PortSpeed::FOURHUNDREDG;
    platformMapping.platformSupportedProfiles()->push_back(profile400G);

    // Add port entries for eth1/1/1 and eth1/2/1 with all supported profiles
    cfg::PlatformPortEntry port1;
    port1.mapping()->name() = "eth1/1/1";
    port1.mapping()->id() = 1;
    port1.supportedProfiles()
        [cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port1.supportedProfiles()
        [cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port1.supportedProfiles()[cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC] =
        cfg::PlatformPortConfig();
    port1.supportedProfiles()
        [cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port1.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91] =
        cfg::PlatformPortConfig();
    port1
        .supportedProfiles()[cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N] =
        cfg::PlatformPortConfig();
    port1
        .supportedProfiles()[cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N] =
        cfg::PlatformPortConfig();
    platformMapping.ports()[1] = port1;

    cfg::PlatformPortEntry port2;
    port2.mapping()->name() = "eth1/2/1";
    port2.mapping()->id() = 2;
    port2.supportedProfiles()
        [cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port2.supportedProfiles()
        [cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port2.supportedProfiles()[cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC] =
        cfg::PlatformPortConfig();
    port2.supportedProfiles()
        [cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port2.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91] =
        cfg::PlatformPortConfig();
    port2
        .supportedProfiles()[cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N] =
        cfg::PlatformPortConfig();
    port2
        .supportedProfiles()[cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N] =
        cfg::PlatformPortConfig();
    platformMapping.ports()[2] = port2;

    cfg::PlatformPortEntry port3;
    port3.mapping()->name() = "eth1/1/2";
    port3.mapping()->id() = 3;
    port3.supportedProfiles()
        [cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port3.supportedProfiles()
        [cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port3.supportedProfiles()[cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC] =
        cfg::PlatformPortConfig();
    port3.supportedProfiles()
        [cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER] =
        cfg::PlatformPortConfig();
    port3.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91] =
        cfg::PlatformPortConfig();
    port3
        .supportedProfiles()[cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N] =
        cfg::PlatformPortConfig();
    port3
        .supportedProfiles()[cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N] =
        cfg::PlatformPortConfig();
    platformMapping.ports()[3] = port3;

    // Setup mock to return this platform mapping
    EXPECT_CALL(getMockAgent(), getPlatformMapping(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(platformMapping), Return()));

    // Setup mock QSFP service to return empty profiles (fallback to hardware)
    std::map<std::string, std::vector<cfg::PortProfileID>> emptyProfiles;
    EXPECT_CALL(getQsfpService(), getAllPortSupportedProfiles(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(emptyProfiles), Return()));
  }

 protected:
  const std::string cmdPrefix_ = "config interface";
};

// ============================================================================
// InterfacesConfig Validation Tests
// ============================================================================

// Test valid config with port + description
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValidPortAndDescription) {
  setupTestableConfigSession();
  InterfacesConfig config({"eth1/1/1", "description", "My port"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "description");
  EXPECT_EQ(config.getAttributes()[0].second, "My port");
}

// Test valid config with port + mtu
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValidPortAndMtu) {
  setupTestableConfigSession();
  InterfacesConfig config({"eth1/1/1", "mtu", "9000"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "mtu");
  EXPECT_EQ(config.getAttributes()[0].second, "9000");
}

// Test valid config with port + both attributes
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValidPortAndBothAttrs) {
  setupTestableConfigSession();
  InterfacesConfig config(
      {"eth1/1/1", "description", "My port", "mtu", "9000"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 2);
  EXPECT_EQ(config.getAttributes()[0].first, "description");
  EXPECT_EQ(config.getAttributes()[0].second, "My port");
  EXPECT_EQ(config.getAttributes()[1].first, "mtu");
  EXPECT_EQ(config.getAttributes()[1].second, "9000");
}

// Test valid config with multiple ports + attributes
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigMultiplePorts) {
  setupTestableConfigSession();
  InterfacesConfig config(
      {"eth1/1/1", "eth1/2/1", "description", "Uplink ports"});
  EXPECT_EQ(config.getInterfaces().size(), 2);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "description");
  EXPECT_EQ(config.getAttributes()[0].second, "Uplink ports");
}

// Test port only (no attributes) - for subcommand pass-through
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigPortOnly) {
  setupTestableConfigSession();
  InterfacesConfig config({"eth1/1/1"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_FALSE(config.hasAttributes());
  EXPECT_TRUE(config.getAttributes().empty());
}

// Test case-insensitive attribute names
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigCaseInsensitiveAttrs) {
  setupTestableConfigSession();
  InterfacesConfig config({"eth1/1/1", "DESCRIPTION", "Test", "MTU", "9000"});
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 2);
  // Attributes should be normalized to lowercase
  EXPECT_EQ(config.getAttributes()[0].first, "description");
  EXPECT_EQ(config.getAttributes()[1].first, "mtu");
}

// Test empty input throws
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigEmptyThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(InterfacesConfig({}), std::invalid_argument);
}

// Test first token is attribute (no port name)
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigNoPortNameThrows) {
  setupTestableConfigSession();
  try {
    InterfacesConfig config({"description", "My port"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("No interface name provided"));
    EXPECT_THAT(e.what(), HasSubstr("description"));
  }
}

// Test missing value for attribute
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigMissingValueThrows) {
  setupTestableConfigSession();
  try {
    InterfacesConfig config({"eth1/1/1", "description"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Missing value for attribute"));
    EXPECT_THAT(e.what(), HasSubstr("description"));
  }
}

// Test value is actually another attribute (forgot value)
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValueIsAttributeThrows) {
  setupTestableConfigSession();
  try {
    InterfacesConfig config({"eth1/1/1", "description", "mtu"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Missing value for attribute"));
    EXPECT_THAT(e.what(), HasSubstr("description"));
    EXPECT_THAT(e.what(), HasSubstr("mtu"));
  }
}

// Test valid config with port + speed
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValidPortAndSpeed) {
  setupTestableConfigSession();
  InterfacesConfig config({"eth1/1/1", "speed", "100000"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "speed");
  EXPECT_EQ(config.getAttributes()[0].second, "100000");
}

// Test unknown attribute name - unknown attribute must appear AFTER a known
// attribute to trigger the error. Otherwise, unknown tokens are treated as
// port names and fail during port resolution.
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigUnknownAttributeThrows) {
  setupTestableConfigSession();
  try {
    // "foobar" comes after "description", so it's recognized as an attribute
    InterfacesConfig config(
        {"eth1/1/1", "description", "Test", "foobar", "value"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Unknown attribute"));
    EXPECT_THAT(e.what(), HasSubstr("foobar"));
  }
}

// Test non-existent port throws
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigNonExistentPortThrows) {
  setupTestableConfigSession();
  EXPECT_THROW(
      InterfacesConfig({"eth1/99/1", "description", "Test"}),
      std::invalid_argument);
}

// ============================================================================
// CmdConfigInterface::queryClient Tests
// ============================================================================

// Test setting description on a single interface
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetsDescription) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 description \"New description\"");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "description", "New description"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("description"));

  // Verify the description was updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1") {
      EXPECT_EQ(*port.description(), "New description");
    } else {
      // Other ports should be unchanged
      EXPECT_EQ(*port.description(), "original description of " + *port.name());
    }
  }
}

// Test setting MTU on a single interface
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetsMtu) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 mtu 9000");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "mtu", "9000"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("mtu=9000"));

  // Verify the MTU was updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& intfs = *switchConfig.interfaces();
  for (const auto& intf : intfs) {
    if (*intf.name() == "eth1/1/1") {
      EXPECT_EQ(*intf.mtu(), 9000);
    } else {
      // Other interfaces should be unchanged
      EXPECT_EQ(*intf.mtu(), 1500);
    }
  }
}

// Test setting both description and MTU
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetsBothAttributes) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 description \"Updated port\" mtu 9000");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config(
      {"eth1/1/1", "description", "Updated port", "mtu", "9000"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("description"));
  EXPECT_THAT(result, HasSubstr("mtu=9000"));

  // Verify both were updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1") {
      EXPECT_EQ(*port.description(), "Updated port");
    }
  }

  auto& intfs = *switchConfig.interfaces();
  for (const auto& intf : intfs) {
    if (*intf.name() == "eth1/1/1") {
      EXPECT_EQ(*intf.mtu(), 9000);
    }
  }
}

// Test setting attributes on multiple interfaces
TEST_F(CmdConfigInterfaceTestFixture, queryClientMultipleInterfaces) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 eth1/2/1 description \"Shared description\"");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config(
      {"eth1/1/1", "eth1/2/1", "description", "Shared description"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));

  // Verify both descriptions were updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    EXPECT_EQ(*port.description(), "Shared description");
  }
}

// Test no attributes throws (pass-through case)
TEST_F(CmdConfigInterfaceTestFixture, queryClientNoAttributesThrows) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1"});

  EXPECT_THROW(cmd.queryClient(localhost(), config), std::runtime_error);
}

// Test invalid MTU value (non-numeric)
TEST_F(CmdConfigInterfaceTestFixture, queryClientInvalidMtuNonNumeric) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "mtu", "abc"});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid MTU value"));
    EXPECT_THAT(e.what(), HasSubstr("abc"));
  }
}

// Test MTU out of range (too low)
TEST_F(CmdConfigInterfaceTestFixture, queryClientMtuTooLow) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config(
      {"eth1/1/1", "mtu", std::to_string(utils::kMtuMin - 1)});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("out of range"));
  }
}

// Test MTU out of range (too high)
TEST_F(CmdConfigInterfaceTestFixture, queryClientMtuTooHigh) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config(
      {"eth1/1/1", "mtu", std::to_string(utils::kMtuMax + 1)});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("out of range"));
  }
}

// Test MTU boundary values (valid)
TEST_F(CmdConfigInterfaceTestFixture, queryClientMtuBoundaryValid) {
  auto cmd = CmdConfigInterface();

  // Test minimum MTU
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 mtu " + std::to_string(utils::kMtuMin));
  InterfacesConfig configMin(
      {"eth1/1/1", "mtu", std::to_string(utils::kMtuMin)});
  EXPECT_THAT(
      cmd.queryClient(localhost(), configMin),
      HasSubstr("Successfully configured"));

  // Test maximum MTU
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 mtu " + std::to_string(utils::kMtuMax));
  InterfacesConfig configMax(
      {"eth1/1/1", "mtu", std::to_string(utils::kMtuMax)});
  EXPECT_THAT(
      cmd.queryClient(localhost(), configMax),
      HasSubstr("Successfully configured"));
}

// Test setting speed with numeric value (Mbps)
TEST_F(CmdConfigInterfaceTestFixture, queryClientSpeedNumeric) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 speed 100000");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "speed", "100000"});
  auto result = cmd.queryClient(localhost(), config);
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("speed=100000"));
}

// Test setting speed to auto
TEST_F(CmdConfigInterfaceTestFixture, queryClientSpeedAuto) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 speed auto");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "speed", "auto"});
  auto result = cmd.queryClient(localhost(), config);
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("speed=auto"));
}

// Test setting speed with case insensitive auto
TEST_F(CmdConfigInterfaceTestFixture, queryClientSpeedAutoCaseInsensitive) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 speed AUTO");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "speed", "AUTO"});
  auto result = cmd.queryClient(localhost(), config);
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("speed=auto"));
}

// Test setting speed with various common speeds (in Mbps)
TEST_F(CmdConfigInterfaceTestFixture, queryClientSpeedCommonValues) {
  auto cmd = CmdConfigInterface();

  // 10G = 10000 Mbps
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 speed 10000");
  InterfacesConfig config10G({"eth1/1/1", "speed", "10000"});
  EXPECT_THAT(
      cmd.queryClient(localhost(), config10G),
      HasSubstr("Successfully configured"));

  // 25G = 25000 Mbps
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 speed 25000");
  InterfacesConfig config25G({"eth1/1/1", "speed", "25000"});
  EXPECT_THAT(
      cmd.queryClient(localhost(), config25G),
      HasSubstr("Successfully configured"));

  // 40G = 40000 Mbps
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 speed 40000");
  InterfacesConfig config40G({"eth1/1/1", "speed", "40000"});
  EXPECT_THAT(
      cmd.queryClient(localhost(), config40G),
      HasSubstr("Successfully configured"));

  // 400G = 400000 Mbps
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 speed 400000");
  InterfacesConfig config400G({"eth1/1/1", "speed", "400000"});
  EXPECT_THAT(
      cmd.queryClient(localhost(), config400G),
      HasSubstr("Successfully configured"));
}

// Test invalid speed value
TEST_F(CmdConfigInterfaceTestFixture, queryClientSpeedInvalid) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "speed", "invalid"});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid speed value"));
  }
}

// Test invalid numeric speed value
TEST_F(CmdConfigInterfaceTestFixture, queryClientSpeedInvalidNumeric) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "speed", "999999"});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid speed"));
    EXPECT_THAT(e.what(), HasSubstr("999999"));
    EXPECT_THAT(e.what(), HasSubstr("Supported speeds (in Mbps):"));
  }
}

// Test multiple interfaces with speed (configured separately)
TEST_F(CmdConfigInterfaceTestFixture, queryClientMultipleInterfacesSpeed) {
  setupSpeedValidationMocks(); // Setup platform and QSFP mocks for speed
                               // validation
  // Configure first port
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 speed 100000");
  auto cmd1 = CmdConfigInterface();
  InterfacesConfig config1({"eth1/1/1", "speed", "100000"});
  auto result1 = cmd1.queryClient(localhost(), config1);
  EXPECT_THAT(result1, HasSubstr("Successfully configured"));
  EXPECT_THAT(result1, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result1, HasSubstr("speed=100000"));

  // Configure second port
  setupTestableConfigSession(cmdPrefix_, "eth1/2/1 speed 100000");
  auto cmd2 = CmdConfigInterface();
  InterfacesConfig config2({"eth1/2/1", "speed", "100000"});
  auto result2 = cmd2.queryClient(localhost(), config2);
  EXPECT_THAT(result2, HasSubstr("Successfully configured"));
  EXPECT_THAT(result2, HasSubstr("eth1/2/1"));
  EXPECT_THAT(result2, HasSubstr("speed=100000"));
}

// Test multiple interfaces with speed in single command
TEST_F(
    CmdConfigInterfaceTestFixture,
    queryClientMultipleInterfacesSpeedSingleCommand) {
  setupSpeedValidationMocks(); // Setup platform and QSFP mocks for speed
                               // validation
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 eth1/2/1 speed 100000");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "eth1/2/1", "speed", "100000"});
  auto result = cmd.queryClient(localhost(), config);
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));
  EXPECT_THAT(result, HasSubstr("speed=100000"));
}

// Test multiple interfaces where speed is valid for one but not the other
TEST_F(
    CmdConfigInterfaceTestFixture,
    queryClientMultipleInterfacesSpeedMismatch) {
  // Create a custom platform mapping where ports have different capabilities
  cfg::PlatformMapping platformMapping;

  // Add 100G profile
  cfg::PlatformPortProfileConfigEntry profile100G;
  profile100G.factor()->profileID() =
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91;
  profile100G.profile()->speed() = cfg::PortSpeed::HUNDREDG;
  platformMapping.platformSupportedProfiles()->push_back(profile100G);

  // Add 200G profile
  cfg::PlatformPortProfileConfigEntry profile200G;
  profile200G.factor()->profileID() =
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N;
  profile200G.profile()->speed() = cfg::PortSpeed::TWOHUNDREDG;
  platformMapping.platformSupportedProfiles()->push_back(profile200G);

  // Port 1: supports only 100G
  cfg::PlatformPortEntry port1;
  port1.mapping()->name() = "eth1/1/1";
  port1.mapping()->id() = 1;
  port1.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91] =
      cfg::PlatformPortConfig();
  platformMapping.ports()[1] = port1;

  // Port 2: supports only 200G (NOT 100G)
  cfg::PlatformPortEntry port2;
  port2.mapping()->name() = "eth1/2/1";
  port2.mapping()->id() = 2;
  port2.supportedProfiles()[cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N] =
      cfg::PlatformPortConfig();
  platformMapping.ports()[2] = port2;

  // Setup mock to return this platform mapping
  EXPECT_CALL(getMockAgent(), getPlatformMapping(_))
      .WillRepeatedly(DoAll(SetArgReferee<0>(platformMapping), Return()));

  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "eth1/2/1", "speed", "100000"});

  // Should fail because eth1/2/1 doesn't support 100G
  try {
    cmd.queryClient(localhost(), config);
    FAIL()
        << "Expected std::invalid_argument for unsupported speed on eth1/2/1";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid speed"));
    EXPECT_THAT(e.what(), HasSubstr("100000"));
    EXPECT_THAT(e.what(), HasSubstr("eth1/2/1"));
    EXPECT_THAT(e.what(), HasSubstr("Supported speeds (in Mbps):"));
    EXPECT_THAT(e.what(), HasSubstr("200000")); // eth1/2/1 supports 200G
  }
}

// Test combining multiple attributes including speed
TEST_F(CmdConfigInterfaceTestFixture, queryClientMultipleAttributesWithSpeed) {
  setupSpeedValidationMocks(); // Setup platform and QSFP mocks for speed
                               // validation
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 description TestPort mtu 9000 speed 100000");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config(
      {"eth1/1/1",
       "description",
       "TestPort",
       "mtu",
       "9000",
       "speed",
       "100000"});
  auto result = cmd.queryClient(localhost(), config);
  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("description="));
  EXPECT_THAT(result, HasSubstr("TestPort"));
  EXPECT_THAT(result, HasSubstr("mtu=9000"));
  EXPECT_THAT(result, HasSubstr("speed=100000"));
}

// Test printOutput
TEST_F(CmdConfigInterfaceTestFixture, printOutput) {
  auto cmd = CmdConfigInterface();
  std::string successMessage =
      "Successfully configured interface(s) eth1/1/1: description=\"Test\"";

  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
  cmd.printOutput(successMessage);
  std::cout.rdbuf(old);

  EXPECT_EQ(buffer.str(), successMessage + "\n");
}

} // namespace facebook::fboss
