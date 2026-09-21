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

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <folly/File.h>
#include <folly/Synchronized.h>
#include <thrift/lib/cpp2/async/ServerStream.h>

#include "fboss/agent/hw/sai/diag/Repl.h"

namespace facebook::fboss {

// Forward declarations to keep this header impl-agnostic.
// ClientInformation is generated from fboss/agent/if/common.thrift; the .cpp
// pulls in the full type via its own include.
class ClientInformation;

namespace pai_diag_detail {

// Own the master side of a created PTY
struct PtyMaster {
  PtyMaster();
  folly::File file;
  std::string name;
};

// Own the slave side of a created PTY
struct PtySlave {
  explicit PtySlave(const PtyMaster& ptyMaster);
  folly::File file;
};

// Manage the lifetime of settings for running a shell session
struct TerminalSession {
  TerminalSession(
      const PtySlave& ptySlave,
      const std::vector<folly::File>& streams);
  ~TerminalSession() noexcept;
  TerminalSession(const TerminalSession&) = delete;
  TerminalSession& operator=(const TerminalSession&) = delete;
  TerminalSession(TerminalSession&&) = delete;
  TerminalSession& operator=(TerminalSession&&) = delete;

  std::map<int, folly::File> fd2oldStreams_;
};

} // namespace pai_diag_detail

/*
 * PaiDiagShell - Interactive shell access to Broadcom Agera3 retimer chips
 * via the PAI (PHY Abstraction Interface) library's built-in diag shell.
 *
 * Mirrors fboss/agent/hw/sai/diag/DiagShell but is decoupled from SaiSwitch*,
 * because qsfp_service's SaiPhyManager owns multiple SaiSwitch instances (one
 * per chip) and we just need a SwitchSaiId (passed as opaque uint64_t in this
 * impl-agnostic header) to drive the SAI attribute that triggers the PAI
 * shell.
 *
 * The PAI library shell supports multi-chip context switching via its
 * built-in `s <switch_id>` command, so a single shell session can hop between
 * all 32 retimer chips on Ladakh800bcls without instantiating multiple shells.
 */
class PaiDiagShell {
 public:
  // switchId is the SAI switch object ID (sai_object_id_t). Passed as
  // uint64_t so this header stays impl-agnostic.
  explicit PaiDiagShell(uint64_t switchId);
  virtual ~PaiDiagShell() noexcept = default;

  PaiDiagShell(const PaiDiagShell&) = delete;
  PaiDiagShell& operator=(const PaiDiagShell&) = delete;
  PaiDiagShell(PaiDiagShell&&) = delete;
  PaiDiagShell& operator=(PaiDiagShell&&) = delete;

  void consumeInput(
      std::unique_ptr<std::string> input,
      std::unique_ptr<ClientInformation> client);

  std::string readOutput(int timeoutMs);
  bool tryConnect();
  void disconnect();
  std::string getPrompt() const;

 protected:
  void initTerminal();

  std::mutex paiDiagShellMutex_;
  std::unique_lock<std::mutex> paiDiagShellLock_;

  // Buffer to read into from pty master side
  std::array<char, 5120> producerBuffer_{};

 private:
  int getPtymFd() const;

  // [[maybe_unused]]: real impl uses this in initTerminal() to construct
  // SaiRepl{SwitchSaiId{switchId_}}; the stub impl in PaiDiagShellStub.cpp
  // throws on every method and never reads the field.
  [[maybe_unused]] uint64_t switchId_{};
  std::unique_ptr<pai_diag_detail::PtyMaster> ptym_;
  std::unique_ptr<pai_diag_detail::PtySlave> ptys_;
  std::unique_ptr<Repl> repl_;
  std::unique_ptr<pai_diag_detail::TerminalSession> ts_;
};

class StreamingPaiDiagShell : public PaiDiagShell {
 public:
  explicit StreamingPaiDiagShell(uint64_t switchId);

  std::string start(
      apache::thrift::ServerStreamPublisher<std::string>&& publisher);
  void markResetPublisher();
  void consumeInput(
      std::unique_ptr<std::string> input,
      std::unique_ptr<ClientInformation> client);

 private:
  void setPublisher(
      apache::thrift::ServerStreamPublisher<std::string>&& publisher);
  void resetPublisher();
  void streamOutput();

  std::unique_ptr<std::thread> producerThread_;
  folly::Synchronized<
      std::unique_ptr<apache::thrift::ServerStreamPublisher<std::string>>,
      std::mutex>
      publisher_;
  std::atomic<bool> shouldResetPublisher_ = false;
};

class PaiDiagCmdServer {
 public:
  explicit PaiDiagCmdServer(PaiDiagShell* paiDiagShell);
  ~PaiDiagCmdServer() noexcept;

  PaiDiagCmdServer(const PaiDiagCmdServer&) = delete;
  PaiDiagCmdServer& operator=(const PaiDiagCmdServer&) = delete;
  PaiDiagCmdServer(PaiDiagCmdServer&&) = delete;
  PaiDiagCmdServer& operator=(PaiDiagCmdServer&&) = delete;

  // One-shot request/response: connect, send command, read output, disconnect
  std::string paiDiagCmd(
      std::unique_ptr<std::string> input,
      std::unique_ptr<ClientInformation> client);

 private:
  std::string produceOutput(int timeoutMs = 0);
  std::string getDelimiterDiagCmd(const std::string& UUID) const;
  std::string& cleanUpOutput(std::string& output, const std::string& input);

  // [[maybe_unused]]: real impl forwards calls to paiDiagShell_; the stub
  // impl in PaiDiagShellStub.cpp throws on every method and never reads
  // the field.
  [[maybe_unused]] PaiDiagShell* paiDiagShell_;
  std::string
      uuid_; // UUID used to delimit per-command output (regenerated per call)
};

class PhyManager;

/*
 * Helper that walks a PhyManager (expected to be a SaiPhyManager) and returns
 * the SAI switch object ID of the first available XPHY chip.
 *
 * Lives in this header so qsfp_service's impl-agnostic handler library can
 * call it without including SAI headers. The actual implementation is in
 * the per-impl pai-diag-shell-{impl} library and downcasts PhyManager to
 * SaiPhyManager at runtime.
 *
 * Throws FbossError if no SAI XPHY chip is available.
 */
uint64_t getFirstSaiSwitchIdForPaiDiagShell(PhyManager* phyMgr);

} // namespace facebook::fboss
