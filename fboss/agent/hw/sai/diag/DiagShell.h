/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <mutex>
#include <string>
#include <thread>

#include <folly/File.h>
#include <folly/Synchronized.h>
#include <thrift/lib/cpp2/async/ServerStream.h>
#include "fboss/agent/hw/sai/switch/gen-cpp2/SaiCtrl.h"

namespace facebook::fboss {

class Repl;
class SaiSwitch;

namespace detail {

// Own the master side of a created PTY
struct PtyMaster {
  // Could throw std::system_error if pty calls fail
  PtyMaster();
  folly::File file;
  std::string name;
};

// Own the slave side of a created PTY
struct PtySlave {
  // Could throw std::system_error if open() fails
  explicit PtySlave(const PtyMaster& ptyMaster);
  folly::File file;
};

// Manage the lifetime of settings for running a shell session
struct TerminalSession {
  TerminalSession(
      const PtySlave& ptySlave,
      const std::vector<folly::File>& streams);
  ~TerminalSession() noexcept;

  // Track the streams we change for this terminal session
  std::vector<folly::File> oldStreams_;
};

// Separate sessions for each connection
class DiagShellClientState {
 public:
  DiagShellClientState(
      const std::string& clientAddrAndPort,
      apache::thrift::ServerStreamPublisher<std::string>&& publisher);

  void publishOutput(const std::string& output);
  void completeStream();

 private:
  const std::string clientAddrAndPort_;
  apache::thrift::ServerStreamPublisher<std::string> publisher_;
};

} // namespace detail

class DiagShell {
 public:
  explicit DiagShell(const SaiSwitch* hw);
  ~DiagShell() noexcept;

  void consumeInput(
      std::unique_ptr<std::string> input,
      std::unique_ptr<ClientInformation> client);

  void setPublisher(
      apache::thrift::ServerStreamPublisher<std::string>&& publisher);
  void markResetPublisher();
  void resetPublisher();
  bool hasPublisher() const;

  std::string getPrompt() const;

  std::string start(
      apache::thrift::ServerStreamPublisher<std::string>&& publisher);

 private:
  void produceOutput();
  std::string getClientInformationStr(
      std::unique_ptr<ClientInformation> clientInfo) const;
  void logToScuba(
      std::unique_ptr<ClientInformation> client,
      const std::string& input,
      const std::optional<std::string>& result = std::nullopt) const;
  std::unique_ptr<Repl> makeRepl() const;

  std::unique_ptr<detail::PtyMaster> ptym_;
  std::unique_ptr<detail::PtySlave> ptys_;
  std::unique_ptr<detail::TerminalSession> ts_;

  folly::Synchronized<
      std::unique_ptr<apache::thrift::ServerStreamPublisher<std::string>>,
      std::mutex>
      publisher_;

  std::unique_ptr<std::thread> producerThread_;

  // Buffer to read into from pty master side
  std::array<char, 512> producerBuffer_;

  std::unique_ptr<Repl> repl_;
  const SaiSwitch* hw_;
  bool shouldResetPublisher_ = false;

  // Condition variable for producer thread
  std::condition_variable producerCV_;

  // Mutex to control when producing should happen
  std::mutex producerMutex_;
};

} // namespace facebook::fboss
