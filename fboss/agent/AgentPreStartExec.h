// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {

class AgentDirectoryUtil;

class AgentPreStartExec {
 public:
  AgentPreStartExec() {}
  void run();
  void run(const AgentDirectoryUtil& dirUtil);
};

} // namespace facebook::fboss
