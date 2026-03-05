// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <fstream>

#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"

namespace facebook::fboss {

CmdConfigTestBase::CmdConfigTestBase(
    const std::string& uniquePath,
    const std::string& initialConfigContent)
    : uniquePath_(uniquePath),
      initialConfigContent_(initialConfigContent),
      testHomeDir_(
          std::filesystem::temp_directory_path() /
          (boost::filesystem::unique_path(uniquePath).string() + "_home")),
      testEtcDir_(
          std::filesystem::temp_directory_path() /
          (boost::filesystem::unique_path(uniquePath).string() + "_etc")),
      cliConfigDir_(testEtcDir_ / "coop" / "cli"),
      git_((testEtcDir_ / "coop").string()) {
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
  std::filesystem::create_directories(cliConfigDir_);

  // Set environment variables
  setenv("HOME", testHomeDir_.c_str(), 1);
  setenv("USER", "testuser", 1);

  // Create a test system config file
  std::filesystem::path cliConfigPath = cliConfigDir_ / "agent.conf";
  createTestConfig(cliConfigPath, initialConfigContent_);

  // Create symlink
  systemConfigPath_ = testEtcDir_ / "coop" / "agent.conf";
  std::filesystem::create_symlink(cliConfigPath, systemConfigPath_);

  // Create session config path
  sessionConfigPath_ = testHomeDir_ / ".fboss2" / "agent.conf";

  // Initialize Git repository and create initial commit
  git_.init();
  git_.commit({"cli/agent.conf"}, "Initial commit");
}

void CmdConfigTestBase::SetUp() {
  CmdHandlerTestBase::SetUp();
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
          (testHomeDir_ / ".fboss2").string(),
          (testEtcDir_ / "coop").string()));
}

void CmdConfigTestBase::setupTestableConfigSession(
    const std::string& cmdPrefix,
    const std::string& cmdArgs) {
  auto testSession = std::make_unique<TestableConfigSession>(
      (testHomeDir_ / ".fboss2").string(), (testEtcDir_ / "coop").string());
  testSession->setCommandLine(fmt::format("{} {}", cmdPrefix, cmdArgs));
  folly::split(' ', cmdArgs, cmdArgsList_, true); // true = ignore empty
  TestableConfigSession::setInstance(std::move(testSession));
}

void CmdConfigTestBase::removeInitialRevisionFile() {
  std::filesystem::remove(cliConfigDir_ / "agent.conf");
  std::filesystem::remove(systemConfigPath_);
  createTestConfig(systemConfigPath_, initialConfigContent_);
}
} // namespace facebook::fboss
