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
#include <thrift/lib/cpp2/async/StreamPublisher.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <thread>

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

} // namespace detail

DiagShell::DiagShell(const SaiSwitch* hw) : hw_(hw) {
  // Set up PTY
  ptym_ = std::make_unique<detail::PtyMaster>();
  ptys_ = std::make_unique<detail::PtySlave>(*ptym_);

  // Start listening on pty master
  producerThread_ =
      std::make_unique<std::thread>([this]() { produceOutput(); });
}

void DiagShell::setPublisher(
    apache::thrift::StreamPublisher<std::string>&& publisher) {
  if (!repl_) {
    // Set up REPL on first thrift connect
    repl_ = makeRepl();
    ts_ =
        std::make_unique<detail::TerminalSession>(*ptys_, repl_->getStreams());
    repl_->run();
  }

  publisher_ = std::make_unique<apache::thrift::StreamPublisher<std::string>>(
      std::move(publisher));
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

void DiagShell::resetPublisher() {
  {
    auto locked = publisher_.lock();
    std::move(**locked).complete();
    locked->reset();
  }
}

bool DiagShell::hasPublisher() const {
  {
    auto locked = publisher_.lock();
    return static_cast<bool>(*locked);
  }
}

// TODO: Customize
std::string DiagShell::getPrompt() const {
  return repl_->getPrompt();
}

DiagShell::~DiagShell() noexcept {
  if (producerThread_) {
    producerThread_->detach();
  }
}

void DiagShell::consumeInput(std::unique_ptr<std::string> input) {
  std::size_t ret =
      folly::writeFull(ptym_->file.fd(), input->c_str(), input->size() + 1);
  folly::checkUnixError(ret, "Failed to write diag shell input to PTY master");
}

void DiagShell::produceOutput() {
  std::size_t nread;
  while (true) {
    nread = ::read(
        ptym_->file.fd(), producerBuffer_.data(), producerBuffer_.size());
    folly::checkUnixError(nread, "Failed to read from the pty master");
    if (nread == 0) {
      XLOG(INFO) << "read 0 bytes from PTY slave; shutting off diag shell";
      break;
    } else {
      // publish string on stream
      if (hasPublisher()) {
        auto locked = publisher_.lock();
        std::string toPublish(producerBuffer_.data(), nread);
        (*locked)->next(toPublish);
      } else {
        XLOG(WARNING) << "Received diag shell output with no user connected";
      }
    }
  }
}

} // namespace facebook::fboss
