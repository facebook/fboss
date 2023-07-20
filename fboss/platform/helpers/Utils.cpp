// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/Utils.h"
#include <exprtk.hpp>
#include <folly/Subprocess.h>
#include <folly/system/Shell.h>
#include <re2/re2.h>
#include <chrono>
#include <unordered_set>

namespace {
const std::unordered_set<std::string> kFlashType = {
    "MX25L12805D",
    "N25Q128..3E",
};

using namespace folly::literals::shell_literals;

std::string execCommandImpl(const std::string& cmd, int* exitStatus) {
  auto shellCmd = "/bin/sh -c {}"_shellify(cmd);
  folly::Subprocess p(shellCmd, folly::Subprocess::Options().pipeStdout());
  auto result = p.communicate();
  if (exitStatus) {
    *exitStatus = p.wait().exitStatus();
  } else {
    p.waitChecked();
  }
  return result.first;
}

} // namespace
namespace facebook::fboss::platform::helpers {

std::string execCommand(const std::string& cmd) {
  return execCommandImpl(cmd, nullptr);
}

std::string execCommandUnchecked(const std::string& cmd, int& exitStatus) {
  return execCommandImpl(cmd, &exitStatus);
}

std::string getFlashType(const std::string& str) {
  for (auto& it : kFlashType) {
    if (str.find(it) != std::string::npos) {
      return it;
    }
  }
  return "";
}

uint64_t nowInSecs() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

float computeExpression(
    const std::string& equation,
    float input,
    const std::string& symbol) {
  std::string temp_equation = equation;

  // Replace "@" with a valid symbol
  static const re2::RE2 atRegex("@");

  re2::RE2::GlobalReplace(&temp_equation, atRegex, symbol);

  exprtk::symbol_table<float> symbolTable;

  symbolTable.add_variable(symbol, input);

  exprtk::expression<float> expr;
  expr.register_symbol_table(symbolTable);

  exprtk::parser<float> parser;
  parser.compile(temp_equation, expr);

  return expr.value();
}

} // namespace facebook::fboss::platform::helpers
