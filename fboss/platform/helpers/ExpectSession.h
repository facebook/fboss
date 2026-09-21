// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <chrono>
#include <string>

namespace facebook::fboss::platform {

/**
 * PTY-based expect session. Spawns a command in a pseudo-terminal and provides
 * expect-style interaction (send/expect pattern matching).
 */
class ExpectSession {
 public:
  explicit ExpectSession(const std::string& command);
  ~ExpectSession();

  ExpectSession(const ExpectSession&) = delete;
  ExpectSession& operator=(const ExpectSession&) = delete;
  ExpectSession(ExpectSession&&) = delete;
  ExpectSession& operator=(ExpectSession&&) = delete;

  // Wait for pattern to appear in output. Returns true if found before timeout.
  bool expect(
      const std::string& pattern,
      std::chrono::milliseconds timeout = std::chrono::seconds(30));

  // Send a command followed by newline
  void sendLine(const std::string& cmd);

  // Send raw data without newline
  void send(const std::string& data);

  // Get buffered output since last successful expect match
  std::string getOutput() const;

  // Check if the child process is still running
  bool isAlive() const;

 private:
  void readAvailable(std::chrono::milliseconds timeout);

  int masterFd_{-1};
  pid_t childPid_{-1};
  bool eof_{false};
  std::string buffer_;
  std::string lastOutput_;
};

} // namespace facebook::fboss::platform
