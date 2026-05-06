// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

namespace facebook::fboss::utils {

// Gates a destructive command behind an interactive y/N confirmation.
//
// Behavior:
//  - If the user passed the `yesFlagName` flag (registered as a LocalOption
//    with isFlag=true), returns immediately.
//  - If stdin or stderr is not a TTY, throws std::runtime_error — refuses to
//    silently mutate state from scripts/cron without explicit opt-in.
//  - Otherwise, prints `warning` and a confirmation line including `target`
//    to stderr, reads a line from stdin, and proceeds only if the user
//    typed "yes" (case-insensitive). Anything else throws std::runtime_error
//    so the audit log records the invocation as FAILURE.
//
// `commandName` and `yesFlagName` must match the strings registered in the
// command's CmdTraits::LocalOptions (so the CmdLocalOptions singleton lookup
// finds the parsed value).
void requireConfirmation(
    const std::string& commandName,
    const std::string& yesFlagName,
    const std::string& warning,
    const std::string& target);

} // namespace facebook::fboss::utils
