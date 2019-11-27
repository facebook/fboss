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
#include <thrift/lib/cpp2/async/StreamPublisher.h>

namespace facebook::fboss {

class Repl;

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

} // namespace detail

class DiagShell {
 public:
  DiagShell();
  ~DiagShell() noexcept;

  void consumeInput(std::unique_ptr<std::string> input);

  void setPublisher(apache::thrift::StreamPublisher<std::string>&& publisher);
  void resetPublisher();
  bool hasPublisher() const;

  std::string getPrompt() const;

 private:
  void produceOutput();

  std::unique_ptr<detail::PtyMaster> ptym_;
  std::unique_ptr<detail::PtySlave> ptys_;
  std::unique_ptr<detail::TerminalSession> ts_;

  folly::Synchronized<
      std::unique_ptr<apache::thrift::StreamPublisher<std::string>>,
      std::mutex>
      publisher_;

  std::unique_ptr<std::thread> producerThread_;

  // Buffer to read into from pty master side
  std::array<char, 512> producerBuffer_;

  std::unique_ptr<Repl> repl_;
};

} // namespace facebook::fboss
