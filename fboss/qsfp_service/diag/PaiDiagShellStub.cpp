/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  Stub implementations of PaiDiagShell.h symbols for builds that don't have
 *  Broadcom PAI (e.g. Credo-only qsfp_service binaries, or any native-PHY
 *  qsfp_service variant).
 *
 *  Why this exists:
 *  QsfpServiceHandler.cpp references PaiDiagShell types so it can implement
 *  the `startPaiDiagShell` / `producePaiDiagShellInput` / `paiDiagCmd` thrift
 *  methods on every qsfp_service variant. The real implementation (which uses
 *  SaiRepl + SaiPhyManager) lives in pai-diag-shell-{brcm_pai_impl} per-impl
 *  libraries. Non-BRCM-PAI binaries link THIS stub instead so handler.so's
 *  unresolved symbols are satisfied at the final binary link stage.
 *
 *  Runtime behavior on Credo / native-PHY binaries: every PaiDiagShell method
 *  throws FbossError("Not supported on this platform"), which thrift turns
 *  into a clean error to the client.
 */
#include "fboss/qsfp_service/diag/PaiDiagShell.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

namespace pai_diag_detail {

// Empty stubs for the PTY/TerminalSession types — they're members of
// PaiDiagShell, so even the constructor needs to link cleanly. We never
// actually create these on stub builds (PaiDiagShell ctor throws first).
PtyMaster::PtyMaster() {
  throw FbossError("PAI diag shell not supported on this platform");
}
PtySlave::PtySlave(const PtyMaster& /*ptyMaster*/) {
  throw FbossError("PAI diag shell not supported on this platform");
}
TerminalSession::TerminalSession(
    const PtySlave& /*ptySlave*/,
    const std::vector<folly::File>& /*streams*/) {
  // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
  throw FbossError("PAI diag shell not supported on this platform");
}
TerminalSession::~TerminalSession() noexcept {}

} // namespace pai_diag_detail

PaiDiagShell::PaiDiagShell(uint64_t /*switchId*/) {
  // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
  throw FbossError(
      "PAI diag shell not supported on this platform "
      "(only available on Broadcom Agera3 retimer builds)");
}

bool PaiDiagShell::tryConnect() {
  throw FbossError("PAI diag shell not supported on this platform");
}

void PaiDiagShell::disconnect() {}

std::string PaiDiagShell::getPrompt() const {
  throw FbossError("PAI diag shell not supported on this platform");
}

void PaiDiagShell::consumeInput(
    std::unique_ptr<std::string> /*input*/,
    std::unique_ptr<ClientInformation> /*client*/) {
  // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
  throw FbossError("PAI diag shell not supported on this platform");
}

std::string PaiDiagShell::readOutput(int /*timeoutMs*/) {
  throw FbossError("PAI diag shell not supported on this platform");
}

void PaiDiagShell::initTerminal() {
  // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
  throw FbossError("PAI diag shell not supported on this platform");
}

int PaiDiagShell::getPtymFd() const {
  throw FbossError("PAI diag shell not supported on this platform");
}

StreamingPaiDiagShell::StreamingPaiDiagShell(uint64_t switchId)
    : PaiDiagShell(switchId) {}

std::string StreamingPaiDiagShell::start(
    apache::thrift::ServerStreamPublisher<std::string>&& /*publisher*/) {
  throw FbossError("PAI diag shell not supported on this platform");
}

void StreamingPaiDiagShell::markResetPublisher() {}

void StreamingPaiDiagShell::consumeInput(
    std::unique_ptr<std::string> /*input*/,
    std::unique_ptr<ClientInformation> /*client*/) {
  // NOLINTNEXTLINE(clang-diagnostic-missing-noreturn)
  throw FbossError("PAI diag shell not supported on this platform");
}

void StreamingPaiDiagShell::setPublisher(
    apache::thrift::ServerStreamPublisher<std::string>&& /*publisher*/) {}

void StreamingPaiDiagShell::resetPublisher() {}

void StreamingPaiDiagShell::streamOutput() {}

PaiDiagCmdServer::PaiDiagCmdServer(PaiDiagShell* paiDiagShell)
    : paiDiagShell_(paiDiagShell) {}

PaiDiagCmdServer::~PaiDiagCmdServer() noexcept {}

std::string PaiDiagCmdServer::paiDiagCmd(
    std::unique_ptr<std::string> /*input*/,
    std::unique_ptr<ClientInformation> /*client*/) {
  throw FbossError("PAI diag shell not supported on this platform");
}

std::string PaiDiagCmdServer::produceOutput(int /*timeoutMs*/) {
  throw FbossError("PAI diag shell not supported on this platform");
}

std::string PaiDiagCmdServer::getDelimiterDiagCmd(
    const std::string& /*UUID*/) const {
  return "";
}

std::string& PaiDiagCmdServer::cleanUpOutput(
    std::string& output,
    const std::string& /*input*/) {
  return output;
}

uint64_t getFirstSaiSwitchIdForPaiDiagShell(PhyManager* /*phyMgr*/) {
  throw FbossError(
      "PAI diag shell not supported on this platform "
      "(only available on Broadcom Agera3 retimer builds)");
}

} // namespace facebook::fboss
