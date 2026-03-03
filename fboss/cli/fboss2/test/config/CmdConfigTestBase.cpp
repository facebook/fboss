// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <fstream>

#include "fboss/cli/fboss2/test/TestableConfigSession.h"

namespace facebook::fboss {

const std::string CmdConfigTestBase::initialAgentCliConfigFile =
    "agent-r1.conf";

void CmdConfigTestBase::SetUp() {
  CmdHandlerTestBase::SetUp();

  // Create unique test directories
  auto tempBase = std::filesystem::temp_directory_path();
  auto uniquePath = boost::filesystem::unique_path(uniquePath_);
  testHomeDir_ = tempBase / (uniquePath.string() + "_home");
  testEtcDir_ = tempBase / (uniquePath.string() + "_etc");

  std::error_code ec;
  if (std::filesystem::exists(testHomeDir_)) {
    std::filesystem::remove_all(testHomeDir_, ec);
  }
  if (std::filesystem::exists(testEtcDir_)) {
    std::filesystem::remove_all(testEtcDir_, ec);
  }

  // Create test directories
  std::filesystem::create_directories(testHomeDir_);
  std::filesystem::create_directories(testEtcDir_ / "coop");

  cliConfigDir_ = testEtcDir_ / "coop" / "cli";
  std::filesystem::create_directories(cliConfigDir_);

  // Set environment variables
  setenv("HOME", testHomeDir_.c_str(), 1);
  setenv("USER", "testuser", 1);

  // Create a test system config file
  std::filesystem::path initialRevision =
      testEtcDir_ / "coop" / "cli" / initialAgentCliConfigFile;
  createTestConfig(initialRevision, initialConfigContent_);

  // Create symlink
  systemConfigPath_ = testEtcDir_ / "coop" / "agent.conf";
  std::filesystem::create_symlink(initialRevision, systemConfigPath_);

  // Create session config path
  sessionConfigPath_ = testHomeDir_ / ".fboss2" / "agent.conf";
}

void CmdConfigTestBase::TearDown() {
  // Reset the singleton to ensure tests don't interfere with each other
  TestableConfigSession::setInstance(nullptr);

  std::error_code ec;
  if (std::filesystem::exists(testHomeDir_)) {
    std::filesystem::remove_all(testHomeDir_, ec);
  }
  if (std::filesystem::exists(testEtcDir_)) {
    std::filesystem::remove_all(testEtcDir_, ec);
  }
  CmdHandlerTestBase::TearDown();
}

void CmdConfigTestBase::createTestConfig(
    const std::filesystem::path& path,
    const std::string& content) {
  std::ofstream file(path);
  file << content;
  file.close();
}

std::string CmdConfigTestBase::readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void CmdConfigTestBase::setupTestableConfigSession() {
  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigPath_.string(),
          systemConfigPath_.string(),
          cliConfigDir_.string()));
}

void CmdConfigTestBase::setupTestableConfigSession(
    const std::string& cmdPrefix,
    const std::string& cmdArgs) {
  auto testSession = std::make_unique<TestableConfigSession>(
      sessionConfigPath_.string(),
      systemConfigPath_.string(),
      cliConfigDir_.string());
  testSession->setCommandLine(fmt::format("{} {}", cmdPrefix, cmdArgs));
  folly::split(' ', cmdArgs, cmdArgsList_, true); // true = ignore empty
  TestableConfigSession::setInstance(std::move(testSession));
}

void CmdConfigTestBase::removeInitialRevisionFile() {
  std::filesystem::remove(cliConfigDir_ / initialAgentCliConfigFile);
  std::filesystem::remove(systemConfigPath_);
  createTestConfig(systemConfigPath_, initialConfigContent_);
}
} // namespace facebook::fboss
