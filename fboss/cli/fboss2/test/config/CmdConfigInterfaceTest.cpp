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
#include <stdexcept>
#include <vector>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/commands/config/interface/ProfileValidation.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

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
    setupProfileValidationMocks();
  }

  void setupProfileValidationMocks() {
    // Create a mock PlatformMapping with common profiles supported
    cfg::PlatformMapping platformMapping;

    // Add platform supported profiles
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

// Test unknown attribute name - unknown attribute must appear AFTER a known
// attribute to trigger the error. Otherwise, unknown tokens are treated as
// port names and fail during port resolution.
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigUnknownAttributeThrows) {
  setupTestableConfigSession();
  try {
    // "speed" comes after "description", so it's recognized as an attribute
    InterfacesConfig config(
        {"eth1/1/1", "description", "Test", "speed", "100000"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Unknown attribute"));
    EXPECT_THAT(e.what(), HasSubstr("speed"));
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

// ============================================================================
// InterfacesConfig Profile Attribute Parsing Tests
// ============================================================================

// Test valid config with port + profile
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValidPortAndProfile) {
  setupTestableConfigSession();
  InterfacesConfig config({"eth1/1/1", "profile", "PROFILE_100G_4_NRZ_CL91"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "profile");
  EXPECT_EQ(config.getAttributes()[0].second, "PROFILE_100G_4_NRZ_CL91");
}

// Test case-insensitive 'profile' attribute name
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigProfileCaseInsensitive) {
  setupTestableConfigSession();

  // Uppercase PROFILE attribute name
  InterfacesConfig config1({"eth1/1/1", "PROFILE", "PROFILE_100G_4_NRZ_CL91"});
  ASSERT_EQ(config1.getAttributes().size(), 1);
  EXPECT_EQ(config1.getAttributes()[0].first, "profile");

  // Mixed case Profile attribute name
  InterfacesConfig config2({"eth1/1/1", "Profile", "PROFILE_100G_4_NRZ_CL91"});
  ASSERT_EQ(config2.getAttributes().size(), 1);
  EXPECT_EQ(config2.getAttributes()[0].first, "profile");
}

// Test unknown attribute after profile value triggers updated error message
TEST_F(
    CmdConfigInterfaceTestFixture,
    interfaceConfigUnknownAttrAfterProfileThrows) {
  setupTestableConfigSession();
  try {
    // "bogus" comes after "profile" + value, so it's recognized as an unknown
    // attribute
    InterfacesConfig config(
        {"eth1/1/1", "profile", "PROFILE_100G_4_NRZ_CL91", "bogus", "val"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Unknown attribute"));
    EXPECT_THAT(e.what(), HasSubstr("bogus"));
    EXPECT_THAT(e.what(), HasSubstr("profile"));
  }
}

// ============================================================================
// ProfileValidator::parseProfile Static Tests
// ============================================================================

// Test valid uppercase profile string
TEST_F(CmdConfigInterfaceTestFixture, parseProfileValidUppercase) {
  auto result =
      ProfileValidator::parseProfile("PROFILE_100G_4_NRZ_RS528_COPPER");
  EXPECT_EQ(result, cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER);
}

// Test valid lowercase profile string (case-insensitive)
TEST_F(CmdConfigInterfaceTestFixture, parseProfileValidLowercase) {
  auto result =
      ProfileValidator::parseProfile("profile_100g_4_nrz_rs528_copper");
  EXPECT_EQ(result, cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER);
}

// Test PROFILE_DEFAULT
TEST_F(CmdConfigInterfaceTestFixture, parseProfileDefault) {
  auto result = ProfileValidator::parseProfile("PROFILE_DEFAULT");
  EXPECT_EQ(result, cfg::PortProfileID::PROFILE_DEFAULT);
}

// Test unknown profile string throws
TEST_F(CmdConfigInterfaceTestFixture, parseProfileUnknown) {
  try {
    ProfileValidator::parseProfile("PROFILE_BOGUS");
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("not a valid PortProfileID"));
    EXPECT_THAT(e.what(), HasSubstr("PROFILE_BOGUS"));
  }
}

// Test empty string throws
TEST_F(CmdConfigInterfaceTestFixture, parseProfileEmpty) {
  EXPECT_THROW(ProfileValidator::parseProfile(""), std::invalid_argument);
}

// ============================================================================
// ProfileValidator::validateProfile Tests (using test constructor)
// ============================================================================

// Test that a supported profile is validated and returned
TEST_F(CmdConfigInterfaceTestFixture, validateProfileSupported) {
  cfg::PlatformMapping platformMapping;

  cfg::PlatformPortProfileConfigEntry profile100G;
  profile100G.factor()->profileID() =
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91;
  profile100G.profile()->speed() = cfg::PortSpeed::HUNDREDG;
  platformMapping.platformSupportedProfiles()->push_back(profile100G);

  cfg::PlatformPortEntry port1;
  port1.mapping()->name() = "eth1/1/1";
  port1.mapping()->id() = 1;
  port1.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91] =
      cfg::PlatformPortConfig();
  platformMapping.ports()[1] = port1;

  std::map<std::string, std::vector<cfg::PortProfileID>> emptyProfiles;
  ProfileValidator validator(platformMapping, emptyProfiles);

  auto result =
      validator.validateProfile("eth1/1/1", "PROFILE_100G_4_NRZ_CL91");
  EXPECT_EQ(result, cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91);
}

// Test PROFILE_DEFAULT is always accepted
TEST_F(CmdConfigInterfaceTestFixture, validateProfileDefault) {
  cfg::PlatformMapping platformMapping;
  std::map<std::string, std::vector<cfg::PortProfileID>> emptyProfiles;
  ProfileValidator validator(platformMapping, emptyProfiles);

  // Even with no port in mapping, PROFILE_DEFAULT should not throw
  auto result = validator.validateProfile("eth1/1/1", "PROFILE_DEFAULT");
  EXPECT_EQ(result, cfg::PortProfileID::PROFILE_DEFAULT);
}

// Test unsupported profile throws std::invalid_argument
TEST_F(CmdConfigInterfaceTestFixture, validateProfileNotSupported) {
  cfg::PlatformMapping platformMapping;

  cfg::PlatformPortEntry port1;
  port1.mapping()->name() = "eth1/1/1";
  port1.mapping()->id() = 1;
  // Only supports 100G
  port1.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91] =
      cfg::PlatformPortConfig();
  platformMapping.ports()[1] = port1;

  std::map<std::string, std::vector<cfg::PortProfileID>> emptyProfiles;
  ProfileValidator validator(platformMapping, emptyProfiles);

  try {
    validator.validateProfile("eth1/1/1", "PROFILE_400G_8_PAM4_RS544X2N");
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid profile"));
    EXPECT_THAT(e.what(), HasSubstr("eth1/1/1"));
    EXPECT_THAT(e.what(), HasSubstr("Supported profiles"));
  }
}

// Test port not found throws std::runtime_error
TEST_F(CmdConfigInterfaceTestFixture, validateProfilePortNotFound) {
  cfg::PlatformMapping platformMapping;
  std::map<std::string, std::vector<cfg::PortProfileID>> emptyProfiles;
  ProfileValidator validator(platformMapping, emptyProfiles);

  try {
    validator.validateProfile("eth99/99/99", "PROFILE_100G_4_NRZ_CL91");
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_THAT(e.what(), HasSubstr("not found"));
  }
}

// Test QSFP profiles take priority over PlatformMapping
TEST_F(CmdConfigInterfaceTestFixture, validateProfileQsfpPrimaryFallback) {
  cfg::PlatformMapping platformMapping;

  // PlatformMapping only has 400G
  cfg::PlatformPortEntry port1;
  port1.mapping()->name() = "eth1/1/1";
  port1.mapping()->id() = 1;
  port1.supportedProfiles()[cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N] =
      cfg::PlatformPortConfig();
  platformMapping.ports()[1] = port1;

  // QSFP says only 100G is supported for this port
  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/1"] = {cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91};

  ProfileValidator validator(platformMapping, qsfpProfiles);

  // Should succeed because QSFP says 100G is supported
  auto result =
      validator.validateProfile("eth1/1/1", "PROFILE_100G_4_NRZ_CL91");
  EXPECT_EQ(result, cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91);

  // Should fail because QSFP does NOT include 400G (even though PlatformMapping
  // does)
  try {
    validator.validateProfile("eth1/1/1", "PROFILE_400G_8_PAM4_RS544X2N");
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid profile"));
  }
}

// ============================================================================
// ProfileValidator::getProfileSpeed Tests
// ============================================================================

// Test getProfileSpeed returns correct speed for known profile
TEST_F(CmdConfigInterfaceTestFixture, getProfileSpeedKnown) {
  cfg::PlatformMapping platformMapping;

  cfg::PlatformPortProfileConfigEntry profile100G;
  profile100G.factor()->profileID() =
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91;
  profile100G.profile()->speed() = cfg::PortSpeed::HUNDREDG;
  platformMapping.platformSupportedProfiles()->push_back(profile100G);

  cfg::PlatformPortEntry port1;
  port1.mapping()->name() = "eth1/1/1";
  port1.mapping()->id() = 1;
  port1.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91] =
      cfg::PlatformPortConfig();
  platformMapping.ports()[1] = port1;

  std::map<std::string, std::vector<cfg::PortProfileID>> emptyProfiles;
  ProfileValidator validator(platformMapping, emptyProfiles);

  auto speed = validator.getProfileSpeed(
      PortID(1), cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91);
  EXPECT_EQ(speed, cfg::PortSpeed::HUNDREDG);
}

// Test getProfileSpeed returns DEFAULT for unknown profile
TEST_F(CmdConfigInterfaceTestFixture, getProfileSpeedUnknown) {
  cfg::PlatformMapping platformMapping;
  std::map<std::string, std::vector<cfg::PortProfileID>> emptyProfiles;
  ProfileValidator validator(platformMapping, emptyProfiles);

  auto speed = validator.getProfileSpeed(
      PortID(1), cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N);
  EXPECT_EQ(speed, cfg::PortSpeed::DEFAULT);
}

// ============================================================================
// CmdConfigInterface::queryClient Profile Tests
// ============================================================================

// Test setting a valid profile on a single interface
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetsProfile) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 profile PROFILE_100G_4_NRZ_CL91");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "profile", "PROFILE_100G_4_NRZ_CL91"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("profile="));
  EXPECT_THAT(result, HasSubstr("speed="));

  // Verify the profile was updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1") {
      EXPECT_EQ(*port.profileID(), cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91);
    }
  }
}

// Test setting PROFILE_DEFAULT resets both fields
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetsProfileDefault) {
  setupTestableConfigSession(cmdPrefix_, "eth1/1/1 profile PROFILE_DEFAULT");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "profile", "PROFILE_DEFAULT"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("profile=PROFILE_DEFAULT"));
  EXPECT_THAT(result, HasSubstr("speed=auto"));

  // Verify both fields were reset
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1") {
      EXPECT_EQ(*port.profileID(), cfg::PortProfileID::PROFILE_DEFAULT);
      EXPECT_EQ(*port.speed(), cfg::PortSpeed::DEFAULT);
    }
  }
}

// Test unsupported profile throws
TEST_F(CmdConfigInterfaceTestFixture, queryClientProfileUnsupportedThrows) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  // PROFILE_100G_4_NRZ_RS528_COPPER is not in our mock PlatformMapping
  InterfacesConfig config(
      {"eth1/1/1", "profile", "PROFILE_100G_4_NRZ_RS528_COPPER"});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid profile"));
    EXPECT_THAT(e.what(), HasSubstr("eth1/1/1"));
    EXPECT_THAT(e.what(), HasSubstr("Supported profiles"));
  }
}

// Test invalid profile string throws before validator is constructed
TEST_F(CmdConfigInterfaceTestFixture, queryClientProfileInvalidStringThrows) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1", "profile", "PROFILE_BOGUS"});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("not a valid PortProfileID"));
    EXPECT_THAT(e.what(), HasSubstr("PROFILE_BOGUS"));
  }
}

// Test setting profile on multiple interfaces
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetProfileMultiPort) {
  setupTestableConfigSession(
      cmdPrefix_, "eth1/1/1 eth1/2/1 profile PROFILE_100G_4_NRZ_CL91");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config(
      {"eth1/1/1", "eth1/2/1", "profile", "PROFILE_100G_4_NRZ_CL91"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));
  EXPECT_THAT(result, HasSubstr("profile="));

  // Verify both ports were updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    EXPECT_EQ(*port.profileID(), cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91);
  }
}

// Test setting profile combined with description
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetProfileAndDescription) {
  setupTestableConfigSession(
      cmdPrefix_,
      "eth1/1/1 profile PROFILE_100G_4_NRZ_CL91 description uplink");
  auto cmd = CmdConfigInterface();
  InterfacesConfig config(
      {"eth1/1/1",
       "profile",
       "PROFILE_100G_4_NRZ_CL91",
       "description",
       "uplink"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("profile="));
  EXPECT_THAT(result, HasSubstr("description="));

  // Verify both were updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1") {
      EXPECT_EQ(*port.profileID(), cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91);
      EXPECT_EQ(*port.description(), "uplink");
    }
  }
}

// Test no-attributes error message mentions profile
TEST_F(
    CmdConfigInterfaceTestFixture,
    queryClientNoAttributesErrorMentionsProfile) {
  setupTestableConfigSession();
  auto cmd = CmdConfigInterface();
  InterfacesConfig config({"eth1/1/1"});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    EXPECT_THAT(e.what(), HasSubstr("profile"));
  }
}

} // namespace facebook::fboss
