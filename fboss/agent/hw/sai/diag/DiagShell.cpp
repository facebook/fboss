/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/diag/DiagShell.h"
#include "fboss/agent/hw/sai/diag/PythonRepl.h"
#include "fboss/agent/hw/sai/diag/SaiRepl.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/Exception.h>
#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <thread>

DEFINE_bool(
    sai_log_to_scribe,
    false,
    "Log SAI shell commands and output to scribe");

namespace facebook::fboss {

namespace detail {

// Helper functions for creating a PTY
folly::File posixOpenpt() {
  int fd = ::posix_openpt(O_RDWR);
  folly::checkUnixError(fd, "Failed to open a new PTY");
  return folly::File(fd, true /* ownsFd */);
}

void grantpt(const folly::File& file) {
  int ret = ::grantpt(file.fd());
  folly::checkUnixError(ret, "Failed to grant access to PTY slave");
}

void unlockpt(const folly::File& file) {
  int ret = ::unlockpt(file.fd());
  folly::checkUnixError(ret, "Failed to unlock PTY slave");
}

std::string ptsnameR(const folly::File& file) {
  std::string name;
  std::array<char, 50> buf;
  int ret = ::ptsname_r(file.fd(), buf.data(), 50);
  folly::checkUnixError(ret, "Failed to get PTY name");
  name = buf.data();
  return name;
}

folly::File dup2(const folly::File& oldFile, const folly::File& newFile) {
  int fd = ::dup2(oldFile.fd(), newFile.fd());
  folly::checkUnixError(
      fd, "Failed to dup2(", oldFile.fd(), ", ", newFile.fd(), ")");
  return folly::File(fd, false /* ownsFd */);
}

PtyMaster::PtyMaster() {
  file = posixOpenpt();
  grantpt(file);
  unlockpt(file);
  name = ptsnameR(file);
}

PtySlave::PtySlave(const PtyMaster& ptyMaster)
    : file(ptyMaster.name.c_str(), O_RDWR) {}

TerminalSession::TerminalSession(
    const PtySlave& ptySlave,
    const std::vector<folly::File>& streams) {
  struct termios oldSettings;
  int ret = ::tcgetattr(ptySlave.file.fd(), &oldSettings);
  folly::checkUnixError(ret, "Failed to get current terminal settings");

  auto newSettings = oldSettings;
  ::cfmakeraw(&newSettings);

  ret = ::tcsetattr(ptySlave.file.fd(), TCSANOW, &newSettings);
  folly::checkUnixError(
      ret, "Failed to set new terminal settings on PTY slave");

  oldStreams_.reserve(streams.size());
  for (const auto& stream : streams) {
    XLOG(INFO) << "Redirect stream to PTY slave: " << stream.fd();
    // Save old stream (OWNING File objects!)
    oldStreams_.emplace_back(stream.dup());
    // Set the pty slave as the stream
    dup2(ptySlave.file, stream);
  }
}

TerminalSession::~TerminalSession() noexcept {
  try {
    for (auto& stream : oldStreams_) {
      // Don't close the stream when we are done
      int fd = stream.release();
      // Restore the old streams
      dup2(stream, folly::File(fd));
    }
  } catch (const std::system_error& e) {
    XLOG(ERR) << "Failed to reset terminal parameters: " << e.what();
  }
}

DiagShellClientState::DiagShellClientState(
    const std::string& clientAddrAndPort,
    apache::thrift::ServerStreamPublisher<std::string>&& publisher)
    : clientAddrAndPort_(clientAddrAndPort), publisher_(std::move(publisher)) {}

void DiagShellClientState::publishOutput(const std::string& output) {
  publisher_.next(output);
}

void DiagShellClientState::completeStream() {
  std::move(publisher_).complete();
  XLOG(INFO) << "Completed Stream on " << clientAddrAndPort_;
}
} // namespace detail

DiagShell::DiagShell(const SaiSwitch* hw) : hw_(hw) {
  // Set up PTY
  ptym_ = std::make_unique<detail::PtyMaster>();
  ptys_ = std::make_unique<detail::PtySlave>(*ptym_);
  diagShellLock_ = std::unique_lock(diagShellMutex_, std::defer_lock);
}

std::unique_ptr<Repl> DiagShell::makeRepl() const {
  switch (hw_->getPlatform()->getMode()) {
    case PlatformMode::WEDGE:
    case PlatformMode::WEDGE100:
    case PlatformMode::GALAXY_LC:
    case PlatformMode::GALAXY_FC:
    case PlatformMode::MINIPACK:
    case PlatformMode::YAMP:
    case PlatformMode::WEDGE400:
    case PlatformMode::FUJI:
    case PlatformMode::ELBERT:
      return std::make_unique<SaiRepl>(hw_->getSwitchId());
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400C_SIM:
      return std::make_unique<PythonRepl>(ptys_->file.fd());
    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
      throw FbossError("Shell not supported for fake platforms");
  }
  CHECK(0) << " Should never get here";
  return nullptr;
}

bool DiagShell::tryConnect() {
  /* Checks if there are no other clients connected.
   * Once this function runs, it will obtain ownership of a lock
   * and block other clients from connecting
   */
  try {
    if (diagShellLock_.try_lock()) {
      return true;
    }
  } catch (const std::system_error& ex) {
    LOG(WARNING) << "Another client already connected";
  }
  throw FbossError(
      "Unable to acquire diag shell lock, client already connected");
}

// TODO: Customize
std::string DiagShell::getPrompt() const {
  return repl_->getPrompt();
}

std::string DiagShell::getDelimiterDiagCmd(const std::string& UUID) const {
  /* Returns the command used for separating each diagCmd,
   * which varies between platform.
   */
  switch (hw_->getPlatform()->getMode()) {
    case PlatformMode::WEDGE:
    case PlatformMode::WEDGE100:
    case PlatformMode::GALAXY_LC:
    case PlatformMode::GALAXY_FC:
    case PlatformMode::MINIPACK:
    case PlatformMode::YAMP:
    case PlatformMode::WEDGE400:
    case PlatformMode::FUJI:
    case PlatformMode::ELBERT:
      return UUID + "\n";
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400C_SIM:
      return folly::to<std::string>("print('", UUID, "')\n");
    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
      throw FbossError("Shell not supported for fake platforms");
  }
  CHECK(0) << " Should never get here";
  return "";
}

void DiagShell::consumeInput(
    std::unique_ptr<std::string> input,
    std::unique_ptr<ClientInformation> client) {
  if (FLAGS_sai_log_to_scribe) {
    logToScuba(std::move(client), *input);
  }
  if (*input == "quit") {
    return;
  }
  std::size_t ret =
      folly::writeFull(ptym_->file.fd(), input->c_str(), input->size() + 1);
  folly::checkUnixError(ret, "Failed to write diag shell input to PTY master");
}

StreamingDiagShellServer::StreamingDiagShellServer(const SaiSwitch* hw)
    : DiagShell(hw) {
  // Create a stream producer thread
  producerThread_ = std::make_unique<std::thread>([this]() { streamOutput(); });
}

StreamingDiagShellServer::~StreamingDiagShellServer() noexcept {
  if (producerThread_) {
    producerThread_->detach();
  }
}

void StreamingDiagShellServer::setPublisher(
    apache::thrift::ServerStreamPublisher<std::string>&& publisher) {
  if (!repl_) {
    // Set up REPL on connect
    repl_ = makeRepl();
    ts_ =
        std::make_unique<detail::TerminalSession>(*ptys_, repl_->getStreams());
    repl_->run();
  }
  publisher_ =
      std::make_unique<apache::thrift::ServerStreamPublisher<std::string>>(
          std::move(publisher));
}

std::string StreamingDiagShellServer::start(
    apache::thrift::ServerStreamPublisher<std::string>&& publisher) {
  assert(diagShellLock_.owns_lock());
  setPublisher(std::move(publisher));
  // We connect to an existing shell (either for the first time or especially
  // on re-connect) so it is necessary to explicitly send the first prompt
  // to the client
  return getPrompt();
}

void StreamingDiagShellServer::markResetPublisher() {
  XLOG(INFO) << "Marked to reset diag shell client states";
  shouldResetPublisher_ = true;
}

void StreamingDiagShellServer::consumeInput(
    std::unique_ptr<std::string> input,
    std::unique_ptr<ClientInformation> client) {
  auto locked = publisher_.lock();
  if (!*locked) {
    std::string msg = "No diag shell connected!";
    XLOG(WARNING) << msg;
    throw FbossError(msg);
  }
  DiagShell::consumeInput(std::move(input), std::move(client));
}

void StreamingDiagShellServer::resetPublisher() {
  XLOG(INFO) << "Resetting diag shell client state";
  auto locked = publisher_.lock();
  if (!shouldResetPublisher_) {
    return;
  }
  if (*locked) {
    std::move(**locked).complete();
  }
  locked->reset();
  // Reset repl and terminal session.
  ts_.reset();
  repl_.reset();
  shouldResetPublisher_ = false;
  diagShellLock_.unlock();
  XLOG(INFO) << "Ready to accept new clients";
}

// TODO: Log command output to Scuba
void StreamingDiagShellServer::streamOutput() {
  std::size_t nread = 0;
  auto fd = ptym_->file.fd();
  while (true) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);
    /* Set the timeout as 500 ms for each read.
     * This is to check that a client has disconnected in the meantime.
     * If the client is still connected, will continue to wait.
     * If the client has disconnected, will clean up the client states.
     */
    struct timeval timeout = {0, 500000};
    if (select(fd + 1, &readSet, nullptr, nullptr, &timeout) > 0) {
      if (!FD_ISSET(fd, &readSet)) {
        std::string msg = "Invalid file descriptor for the PTY";
        XLOG(WARNING) << msg;
        throw FbossError(msg);
      }
      nread = ::read(fd, producerBuffer_.data(), producerBuffer_.size());
      folly::checkUnixError(nread, "Failed to read from the pty master");
      if (nread == 0) {
        XLOG(INFO) << "read 0 bytes from PTY slave; shutting off diag shell";
        break;
      } else {
        // publish string on stream
        std::string toPublish(producerBuffer_.data(), nread);
        auto locked = publisher_.lock();
        if (*locked) {
          (*locked)->next(toPublish);
        } else if (!shouldResetPublisher_) {
          // Warn if there has not been a client but output was produced
          XLOG(WARNING)
              << "Received diag shell output when no client is connected";
        }
      }
    }

    if (shouldResetPublisher_) {
      resetPublisher();
    }
  }
}

} // namespace facebook::fboss
