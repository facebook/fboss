// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmKmodUtils.h"
#include <folly/logging/xlog.h>
#include "common/process/Process.h"

namespace facebook::fboss {

std::string getBcmKmodParam(std::string param) {
  std::string resultStr;
  std::string errStr;
  std::string cmd = "cat /sys/module/linux_user_bde/parameters/" + param;
  bool ret = facebook::process::Process::execShellCmd(cmd, &resultStr, &errStr);
  if (!ret || resultStr.empty()) {
    XLOG(ERR) << "Failed in getting kmod pamameter. Error: " << errStr;
  }
  return resultStr;
}

bool setBcmKmodParam(std::string param, std::string val) {
  std::string resultStr;
  std::string errStr;
  std::string cmd = folly::to<std::string>(
      "echo ", val, " > /sys/module/linux_user_bde/parameters/", param);
  bool ret = facebook::process::Process::execShellCmd(cmd, &resultStr, &errStr);
  if (!ret) {
    XLOG(ERR) << "Failed in setting kmod pamameter. Error: " << errStr;
  }
  return ret;
}

} // namespace facebook::fboss
