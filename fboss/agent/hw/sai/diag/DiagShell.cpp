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
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/lib/platforms/PlatformMode.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
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
    true,
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

  for (const auto& stream : streams) {
    XLOG(DBG2) << "Redirect stream to PTY slave: " << stream.fd();
    // Save old stream (OWNING File objects!)
    fd2oldStreams_.insert({stream.fd(), stream.dup()});
    // Set the pty slave as the stream
    dup2(ptySlave.file, stream);
  }
}

TerminalSession::~TerminalSession() noexcept {
  try {
    // wait 100ms for additional diag output, so as to avoid
    // potential offending write() during dup2() that might
    // cause pty fd got stuck
    /* sleep override */
    usleep(100 * 1000);
    fflush(stdout);
    for (auto& fd2stream : fd2oldStreams_) {
      // Restore the old streams
      dup2(fd2stream.second, folly::File(fd2stream.first));
      XLOG(DBG2) << "Restore stream to standard std fd " << fd2stream.first;
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
  XLOG(DBG2) << "Completed Stream on " << clientAddrAndPort_;
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
    case PlatformMode::DARWIN:
    case PlatformMode::MERU400BIU:
    case PlatformMode::MERU400BIA:
    case PlatformMode::KAMET:
    case PlatformMode::MONTBLANC:
      return std::make_unique<SaiRepl>(hw_->getSaiSwitchId());
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400C_SIM:
    case PlatformMode::WEDGE400C_VOQ:
    case PlatformMode::WEDGE400C_FABRIC:
    case PlatformMode::CLOUDRIPPER:
    case PlatformMode::CLOUDRIPPER_VOQ:
    case PlatformMode::CLOUDRIPPER_FABRIC:
    case PlatformMode::LASSEN:
    case PlatformMode::SANDIA:
      return std::make_unique<PythonRepl>(ptys_->file.fd());
    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
      throw FbossError("Shell not supported for fake platforms");
  }
  CHECK(0) << " Should never get here";
  return nullptr;
}

void DiagShell::initTerminal() {
  if (!repl_) {
    // Set up REPL on connect if needed
    repl_ = makeRepl();
    ts_.reset(new detail::TerminalSession(*ptys_, repl_->getStreams()));
    repl_->run();
  } else if (!ts_) {
    ts_.reset(new detail::TerminalSession(*ptys_, repl_->getStreams()));
  }
}

int DiagShell::getPtymFd() const {
  /* Helper function to return the file descriptor for pty master */
  return ptym_->file.fd();
}

bool DiagShell::tryConnect() {
  /* Checks if there are no other clients connected.
   * Once this function runs, it will obtain ownership of a lock
   * and block other clients from connecting
   */
  try {
    if (diagShellLock_.try_lock()) {
      initTerminal();
      return true;
    }
  } catch (const std::system_error& ex) {
    LOG(WARNING) << "Another diag shell client already connected";
  }
  throw FbossError(
      "Unable to acquire diag shell lock, client already connected");
}

void DiagShell::disconnect() {
  try {
    // TODO: look into restore stdin/stdout after session closed for PythonRepl.
    // Currrently, the following sessions from diag_shell_client on tajo
    // switches might got stuck if terminal session is reset.
    if (hw_->getPlatform()->getAsic()->getAsicVendor() ==
        HwAsic::AsicVendor::ASIC_VENDOR_BCM) {
      ts_.reset();
    }
    diagShellLock_.unlock();
  } catch (const std::system_error& ex) {
    XLOG(WARNING) << "Trying to disconnect when it was never connected";
  }
}

// TODO: Customize
std::string DiagShell::getPrompt() const {
  return repl_->getPrompt();
}

void DiagShell::consumeInput(
    std::unique_ptr<std::string> input,
    std::unique_ptr<ClientInformation> client) {
  if (FLAGS_sai_log_to_scribe) {
    logToScuba(std::move(client), *input);
  }
  if (!repl_) {
    return;
  }
  std::size_t ret =
      folly::writeFull(getPtymFd(), input->c_str(), input->size() + 1);
  folly::checkUnixError(ret, "Failed to write diag shell input to PTY master");
  if (*input == "quit") {
    // TODO: block until repl loop is completed
  }
}

std::string DiagShell::readOutput(int timeoutMs) {
  auto fd = getPtymFd();
  std::string output;
  fd_set readSet;
  FD_ZERO(&readSet);
  FD_SET(fd, &readSet);
  /* Set the timeout as 500 ms for each read.
   * This is to check that a client has disconnected in the meantime.
   * If the client is still connected, will continue to wait.
   * If the client has disconnected, will clean up the client states.
   */
  struct timeval timeout = {0, timeoutMs * 1000};
  // If timeout is < 0, have no timeout and block indefinitely
  timeval* timeoutPtr = (timeoutMs >= 0) ? &timeout : nullptr;
  if (select(fd + 1, &readSet, nullptr, nullptr, timeoutPtr) > 0) {
    if (!FD_ISSET(fd, &readSet)) {
      std::string msg = "Invalid file descriptor for the PTY";
      XLOG(WARNING) << msg;
      throw FbossError(msg);
    }
    int nread = ::read(fd, producerBuffer_.data(), producerBuffer_.size());
    folly::checkUnixError(nread, "Failed to read from the pty master");
    if (nread == 0) {
      std::string msg = "read 0 bytes from PTY slave;";
      XLOG(WARNING) << msg;
      throw FbossError(msg);
    }
    output += std::string(producerBuffer_.data(), nread);
  }
  return output;
}

StreamingDiagShellServer::StreamingDiagShellServer(const SaiSwitch* hw)
    : DiagShell(hw) {}

void StreamingDiagShellServer::setPublisher(
    apache::thrift::ServerStreamPublisher<std::string>&& publisher) {
  publisher_ =
      std::make_unique<apache::thrift::ServerStreamPublisher<std::string>>(
          std::move(publisher));
}

std::string StreamingDiagShellServer::start(
    apache::thrift::ServerStreamPublisher<std::string>&& publisher) {
  assert(diagShellLock_.owns_lock());
  setPublisher(std::move(publisher));
  // Create a stream producer thread
  producerThread_.reset(new std::thread([this]() { streamOutput(); }));
  return getPrompt();
}

void StreamingDiagShellServer::markResetPublisher() {
  XLOG(DBG2) << "Marked to reset diag shell client states";
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
  XLOG(DBG2) << "Resetting diag shell client state";
  auto locked = publisher_.lock();
  if (!shouldResetPublisher_) {
    return;
  }
  if (*locked) {
    std::move(**locked).complete();
  }
  // Detaching the producer thread, since it should already be completed
  if (producerThread_) {
    producerThread_->detach();
  }
  shouldResetPublisher_ = false;
  locked->reset();
  disconnect();
  XLOG(DBG2) << "Ready to accept new clients";
}

// TODO: Log command output to Scuba
void StreamingDiagShellServer::streamOutput() {
  while (!shouldResetPublisher_) {
    /* Set the timeout as 500 ms for each read.
     * This is to check that a client has disconnected in the meantime.
     * If the client is still connected, will continue to wait.
     * If the client has disconnected, will clean up the client states.
     */
    std::string toPublish = readOutput(500);
    if (toPublish.length() > 0) {
      // publish string on stream
      auto locked = publisher_.lock();
      if (*locked) {
        (*locked)->next(toPublish);
      }
    }
  }
  resetPublisher();
}

DiagCmdServer::DiagCmdServer(const SaiSwitch* hw, DiagShell* diagShell)
    : hw_(hw),
      diagShell_(diagShell),
      uuid_(boost::uuids::to_string(boost::uuids::random_generator()())) {}

DiagCmdServer::~DiagCmdServer() noexcept {}

std::string DiagCmdServer::getDelimiterDiagCmd(const std::string& UUID) const {
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
    case PlatformMode::DARWIN:
    case PlatformMode::MERU400BIU:
    case PlatformMode::MERU400BIA:
    case PlatformMode::KAMET:
    case PlatformMode::MONTBLANC:
      return UUID + "\n";
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400C_SIM:
    case PlatformMode::WEDGE400C_VOQ:
    case PlatformMode::WEDGE400C_FABRIC:
    case PlatformMode::CLOUDRIPPER:
    case PlatformMode::CLOUDRIPPER_VOQ:
    case PlatformMode::CLOUDRIPPER_FABRIC:
    case PlatformMode::LASSEN:
    case PlatformMode::SANDIA:
      return folly::to<std::string>("print('", UUID, "')\n");
    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
      throw FbossError("Shell not supported for fake platforms");
  }
  CHECK(0) << " Should never get here";
  return "";
}

std::string& DiagCmdServer::cleanUpOutput(
    std::string& output,
    const std::string& input) {
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
    case PlatformMode::DARWIN:
    case PlatformMode::MERU400BIU:
    case PlatformMode::MERU400BIA:
    case PlatformMode::KAMET:
    case PlatformMode::MONTBLANC:
      // Clean up the back of the string
      if (!output.empty() && !input.empty()) {
        std::string shell = "drivshell>";
        int pos = output.rfind(shell + uuid_);
        if (pos != std::string::npos) {
          // Remove uuid if the output is for an actual command.
          output = output.erase(pos);
        }
        // Cleaning shell inputs
        std::string inputStr = input.substr(0, input.find('\n')) + "\x0d\n";
        pos = output.find(input.substr(0, input.find('\n')) + "\x0d\n");
        if (pos != std::string::npos) {
          output = output.erase(0, pos + inputStr.length());
        }
      }
      return output;
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400C_SIM:
    case PlatformMode::WEDGE400C_VOQ:
    case PlatformMode::WEDGE400C_FABRIC:
    case PlatformMode::LASSEN:
    case PlatformMode::SANDIA:
      return output;
    case PlatformMode::CLOUDRIPPER:
    case PlatformMode::CLOUDRIPPER_VOQ:
    case PlatformMode::CLOUDRIPPER_FABRIC:
      throw FbossError("Shell not supported for cloud ripper platform");
    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
      throw FbossError("Shell not supported for fake platforms");
  }
  CHECK(0) << " Should never get here";
  return output;
}

std::string DiagCmdServer::diagCmd(
    std::unique_ptr<fbstring> input,
    std::unique_ptr<ClientInformation> client) {
  // TODO: Look into adding timeout for try_connect
  // TODO: Look into adding context on which client is connected
  if (!diagShell_->tryConnect()) {
    // TODO: Look into how we can add timeout in future diffs
    throw FbossError("Another client connected");
  }
  std::string inputStr = input->toStdString();
  XLOG(DBG1) << "Connected and running diagCmd: " << inputStr;
  // Flush out old output
  // These should be made in different consumeInput calls,
  // or the input gets mixed up.
  diagShell_->consumeInput(
      std::make_unique<std::string>("\n"), std::move(client));
  produceOutput();
  diagShell_->consumeInput(
      std::make_unique<std::string>(inputStr), std::move(client));
  diagShell_->consumeInput(
      std::make_unique<std::string>(getDelimiterDiagCmd(uuid_)),
      std::move(client));
  // TODO: Look into requesting results that take a long time
  std::string output = produceOutput(500);
  cleanUpOutput(output, inputStr);
  diagShell_->disconnect();
  return output;
}

std::string DiagCmdServer::produceOutput(int timeoutMs) {
  std::ostringstream os;
  int currTimeout = timeoutMs;
  while (true) {
    std::string tmpOutput = diagShell_->readOutput(currTimeout);
    if (tmpOutput.length() == 0) {
      break;
    }
    os << tmpOutput;
    // TODO: Check platform specific termination criteria
    if (tmpOutput.rfind("Unknown command: " + uuid_) != std::string::npos) {
      break;
    }
  }
  return os.str();
}

} // namespace facebook::fboss
