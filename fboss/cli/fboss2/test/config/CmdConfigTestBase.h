// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include <filesystem>

namespace facebook::fboss {

class CmdConfigTestBase : public CmdHandlerTestBase {
 public:
  static const std::string initialAgentCliConfigFile;

  CmdConfigTestBase(
      const std::string& uniquePath,
      const std::string& initialConfigContent)
      : uniquePath_(uniquePath), initialConfigContent_(initialConfigContent) {}

  void SetUp() override;
  void TearDown() override;

  std::filesystem::path getTestHomeDir() const {
    return testHomeDir_;
  }

  std::filesystem::path getTestEtcDir() const {
    return testEtcDir_;
  }

  std::filesystem::path getSystemConfigPath() const {
    return systemConfigPath_;
  }

  std::filesystem::path getSessionConfigPath() const {
    return sessionConfigPath_;
  }

  std::filesystem::path getCliConfigDir() const {
    return cliConfigDir_;
  }
  bool cliConfigDirExists() const {
    return std::filesystem::exists(cliConfigDir_);
  }

  void createTestConfig(
      const std::filesystem::path& path,
      const std::string& content);

  // For now, we only support one command line per TestableConfigSession
  void setupTestableConfigSession();
  void setupTestableConfigSession(
      const std::string& cmdPrefix,
      const std::string& cmdArgs);

  const std::vector<std::string>& getCmdArgsList() const {
    return cmdArgsList_;
  }

  std::string readFile(const std::filesystem::path& path);

  void removeInitialRevisionFile();

 private:
  CmdConfigTestBase() = default;

  // All Config Tests should set these values in their constructors
  std::string uniquePath_{""};
  std::string initialConfigContent_{""};

  std::filesystem::path testHomeDir_;
  std::filesystem::path testEtcDir_;
  std::filesystem::path systemConfigPath_;
  std::filesystem::path sessionConfigPath_;
  std::filesystem::path cliConfigDir_;
  // The following cmd attributes will be set after calling
  // `setupTestableConfigSession()`
  std::string cmdPrefix_;
  std::string cmdArgs_;
  std::vector<std::string> cmdArgsList_;
};
} // namespace facebook::fboss
