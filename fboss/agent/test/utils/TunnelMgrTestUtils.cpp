// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/TunnelMgrTestUtils.h"

#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <folly/Conv.h>
#include <folly/IPAddress.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/TunManager.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss::utility {

void clearKernelEntries(const std::string& intfIp, bool isIPv4) {
  std::string cmd;
  if (isIPv4) {
    cmd = folly::to<std::string>("ip rule list | grep -w ", intfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 rule list | grep -w ", intfIp);
  }

  auto output = runShellCmd(cmd);

  XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearKernelEntries Output: \n" << output;

  while (output.find(folly::to<std::string>(intfIp)) != std::string::npos) {
    std::istringstream iss(output);
    std::string word;
    std::string lastWord;

    while (iss >> word) {
      lastWord = folly::copy(word);
    }

    XLOG(DBG2) << "tableId: " << lastWord;

    if (isIPv4) {
      cmd = folly::to<std::string>("ip rule delete table ", lastWord);
    } else {
      cmd = folly::to<std::string>("ip -6 rule delete table ", lastWord);
    }

    runShellCmd(cmd);

    if (isIPv4) {
      cmd = folly::to<std::string>("ip rule list | grep -w ", intfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 rule list | grep -w ", intfIp);
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearKernelEntries Output: \n" << output;
  }

  if (isIPv4) {
    cmd = folly::to<std::string>("ip route list | grep -w ", intfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 route list | grep -w ", intfIp);
  }

  output = runShellCmd(cmd);

  XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearKernelEntries Output: \n" << output;

  std::istringstream iss(output);
  do {
    std::string subs;
    iss >> subs;
    if (subs.find("fboss") != std::string::npos) {
      if (subs.find(':') != std::string::npos) {
        subs = subs.substr(0, subs.find(':'));
      }
      cmd = folly::to<std::string>("ip link delete ", subs);
      runShellCmd(cmd);
      XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
      break;
    }
  } while (iss);

  if (isIPv4) {
    cmd = folly::to<std::string>("ip addr list | grep -w ", intfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 addr list | grep -w ", intfIp);
  }

  output = runShellCmd(cmd);

  XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearKernelEntries Output: \n" << output;

  std::istringstream iss2(output);
  do {
    std::string subs;
    iss2 >> subs;
    if (subs.find("fboss") != std::string::npos) {
      if (subs.find(':') != std::string::npos) {
        subs = subs.substr(0, subs.find(':'));
      }
      cmd = folly::to<std::string>("ip link delete ", subs);
      runShellCmd(cmd);
      XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
      break;
    }
  } while (iss2);
}

void clearKernelEntries(
    const std::string& intfIPv4,
    const std::string& intfIPv6) {
  clearKernelEntries(intfIPv4, true);
  clearKernelEntries(intfIPv6, false);
}

void clearAllKernelEntries() {
  std::string cmd;
  cmd = folly::to<std::string>("ip rule list");
  auto output = runShellCmd(cmd);

  XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

  std::istringstream iss(output);
  for (std::string line; std::getline(iss, line);) {
    std::stringstream ss(line);
    std::string word;
    std::string lastWord;

    while (ss >> word) {
      lastWord = folly::copy(word);
    }

    if (lastWord != "local" && lastWord != "main" && lastWord != "default") {
      XLOG(DBG2) << "tableId: " << lastWord;
      cmd = folly::to<std::string>("ip rule delete table ", lastWord);
      runShellCmd(cmd);
    }
  }

  cmd = folly::to<std::string>("ip -6 rule list");
  output = runShellCmd(cmd);

  XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

  std::istringstream iss2(output);
  for (std::string line; std::getline(iss2, line);) {
    std::stringstream ss(line);
    std::string word;
    std::string lastWord;

    while (ss >> word) {
      lastWord = folly::copy(word);
    }

    if (lastWord != "local" && lastWord != "main" && lastWord != "default") {
      XLOG(DBG2) << "tableId: " << lastWord;
      cmd = folly::to<std::string>("ip -6 rule delete table ", lastWord);
      runShellCmd(cmd);
    }
  }

  cmd = folly::to<std::string>("ip route list | grep fboss");
  output = runShellCmd(cmd);

  XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

  while (output.find(folly::to<std::string>("fboss")) != std::string::npos) {
    std::istringstream iss3(output);
    do {
      std::string subs;
      iss3 >> subs;
      if (subs.find("fboss") != std::string::npos) {
        if (subs.find(':') != std::string::npos) {
          subs = subs.substr(0, subs.find(':'));
        }
        cmd = folly::to<std::string>("ip link delete ", subs);
        runShellCmd(cmd);
        XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
        break;
      }
    } while (iss3);

    cmd = folly::to<std::string>("ip route list | grep fboss");
    output = runShellCmd(cmd);
  }

  cmd = folly::to<std::string>("ip -6 route list | grep fboss");
  output = runShellCmd(cmd);

  XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

  while (output.find(folly::to<std::string>("fboss")) != std::string::npos) {
    std::istringstream iss4(output);
    do {
      std::string subs;
      iss4 >> subs;
      if (subs.find("fboss") != std::string::npos) {
        if (subs.find(':') != std::string::npos) {
          subs = subs.substr(0, subs.find(':'));
        }
        cmd = folly::to<std::string>("ip link delete ", subs);
        runShellCmd(cmd);
        XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
        break;
      }
    } while (iss4);

    cmd = folly::to<std::string>("ip -6 route list | grep fboss");
    output = runShellCmd(cmd);
  }

  cmd = folly::to<std::string>("ip addr list | grep fboss");
  output = runShellCmd(cmd);

  XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

  while (output.find(folly::to<std::string>("fboss")) != std::string::npos) {
    std::istringstream iss5(output);
    do {
      std::string subs;
      iss5 >> subs;
      if (subs.find("fboss") != std::string::npos) {
        if (subs.find(':') != std::string::npos) {
          subs = subs.substr(0, subs.find(':'));
        }
        cmd = folly::to<std::string>("ip link delete ", subs);
        runShellCmd(cmd);
        XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
        break;
      }
    } while (iss5);

    cmd = folly::to<std::string>("ip addr list | grep fboss");
    output = runShellCmd(cmd);
  }

  cmd = folly::to<std::string>("ip -6 addr list | grep fboss");
  output = runShellCmd(cmd);

  XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
  XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

  while (output.find(folly::to<std::string>("fboss")) != std::string::npos) {
    std::istringstream iss6(output);
    do {
      std::string subs;
      iss6 >> subs;
      if (subs.find("fboss") != std::string::npos) {
        if (subs.find(':') != std::string::npos) {
          subs = subs.substr(0, subs.find(':'));
        }
        cmd = folly::to<std::string>("ip link delete ", subs);
        runShellCmd(cmd);
        XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
        break;
      }
    } while (iss6);
    cmd = folly::to<std::string>("ip -6 addr list | grep fboss");
    output = runShellCmd(cmd);
  }
}

bool checkIpRuleEntriesRemoved(const std::string& intfIp, bool isIPv4) {
  std::string cmd;
  if (isIPv4) {
    cmd = folly::to<std::string>("ip rule list | grep -w ", intfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 rule list | grep -w ", intfIp);
  }

  auto output = runShellCmd(cmd);

  XLOG(DBG2) << "checkIpRuleEntriesRemoved Cmd: " << cmd;
  XLOG(DBG2) << "checkIpRuleEntriesRemoved Output: \n" << output;

  if (output.find(folly::to<std::string>(intfIp)) != std::string::npos) {
    return false;
  }

  return true;
}

void checkIpKernelEntriesRemoved(const std::string& intfIp, bool isIPv4) {
  std::string cmd;
  std::string searchIntfIp = intfIp;
  if (isIPv4) {
    cmd = folly::to<std::string>("ip rule list | grep -w ", searchIntfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 rule list | grep -w ", searchIntfIp);
  }

  auto output = runShellCmd(cmd);

  XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
  XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

  EXPECT_TRUE(
      output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);

  if (isIPv4) {
    cmd = folly::to<std::string>("ip addr list | grep -w ", searchIntfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 addr list | grep -w ", searchIntfIp);
  }

  output = runShellCmd(cmd);

  XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
  XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

  if (output.find(folly::to<std::string>(searchIntfIp)) != std::string::npos) {
    XLOG(DBG2)
        << "checkKernelEntriesRemoved: Tunnel address entry not  removed for "
        << searchIntfIp;
  }

  if (isIPv4) {
    cmd = folly::to<std::string>(
        "ip route list | grep -w ", searchIntfIp, " | grep fboss");
  } else {
    cmd = folly::to<std::string>(
        "ip -6 route list | grep -w ", searchIntfIp, " | grep fboss");
  }

  output = runShellCmd(cmd);

  XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
  XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

  EXPECT_TRUE(
      output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);
}

void checkKernelEntriesNotExist(
    const std::string& intfIp,
    bool isIPv4,
    bool checkRouteEntry) {
  std::string cmd;
  std::string searchIntfIp = intfIp;
  if (isIPv4) {
    cmd = folly::to<std::string>("ip rule list | grep -w ", searchIntfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 rule list | grep -w ", searchIntfIp);
  }

  auto output = runShellCmd(cmd);

  XLOG(DBG2) << "checkKernelEntriesNotExist Cmd: " << cmd;
  XLOG(DBG2) << "checkKernelEntriesNotExist Output: \n" << output;

  EXPECT_TRUE(
      output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);

  if (isIPv4) {
    cmd = folly::to<std::string>("ip addr list | grep -w ", searchIntfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 addr list | grep -w ", searchIntfIp);
  }

  output = runShellCmd(cmd);

  XLOG(DBG2) << "checkKernelEntriesNotExist Cmd: " << cmd;
  XLOG(DBG2) << "checkKernelEntriesNotExist Output: \n" << output;

  EXPECT_TRUE(
      output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);

  if (checkRouteEntry) {
    if (isIPv4) {
      cmd = folly::to<std::string>(
          "ip route list | grep -w ", searchIntfIp, " | grep fboss");
    } else {
      cmd = folly::to<std::string>(
          "ip -6 route list | grep -w ", searchIntfIp, " | grep fboss");
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesNotExist Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesNotExist Output:" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);

    if (isIPv4) {
      cmd = folly::to<std::string>("ip route list table all | grep fboss");
    } else {
      cmd = folly::to<std::string>("ip -6 route list table all | grep fboss");
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesExist Additional Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesExist Additional Output: " << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>("fboss")) == std::string::npos);
  }
}

void checkKernelEntriesExist(
    const std::string& intfIp,
    bool isIPv4,
    bool checkRouteEntry) {
  std::string cmd;
  std::string searchIntfIp = intfIp;
  if (isIPv4) {
    cmd = folly::to<std::string>("ip rule list | grep -w ", searchIntfIp);
  } else {
    cmd = folly::to<std::string>("ip -6 rule list | grep -w ", searchIntfIp);
  }

  auto output = runShellCmd(cmd);

  XLOG(DBG2) << "checkKernelEntriesExist Cmd: " << cmd;
  XLOG(DBG2) << "checkKernelEntriesExist Output: " << std::endl
             << output << std::endl;

  if (output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos) {
    XLOG(DBG2) << "checkKernelEntriesExist: Source route rule entry not "
               << "present in the kernel for " << searchIntfIp;
  }

  if (isIPv4) {
    cmd = folly::to<std::string>("ip addr list | grep -B 1 -w ", searchIntfIp);
  } else {
    cmd =
        folly::to<std::string>("ip -6 addr list | grep -B 1 -w ", searchIntfIp);
  }

  output = runShellCmd(cmd);

  XLOG(DBG2) << "checkKernelEntriesExist Cmd: " << cmd;
  XLOG(DBG2) << "checkKernelEntriesExist Output: " << std::endl
             << output << std::endl;

  EXPECT_TRUE(
      output.find(folly::to<std::string>(searchIntfIp)) != std::string::npos);

  if (checkRouteEntry) {
    if (isIPv4) {
      cmd = folly::to<std::string>(
          "ip route list table all | grep -w ", searchIntfIp, " | grep fboss");
    } else {
      cmd = folly::to<std::string>(
          "ip -6 route list table all | grep -w ",
          searchIntfIp,
          " | grep fboss");
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesExist Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesExist Output:" << std::endl
               << output << std::endl;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) != std::string::npos);

    if (isIPv4) {
      cmd = folly::to<std::string>("ip route list table all | grep fboss");
    } else {
      cmd = folly::to<std::string>("ip -6 route list table all | grep fboss");
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesExist Additional Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesExist Additional Output: \n"
               << output << "\n";

    EXPECT_TRUE(
        output.find(folly::to<std::string>("fboss")) != std::string::npos);
  }
}

void checkKernelEntriesRemoved(
    const std::string& intfIPv4,
    const std::string& intfIPv6) {
  checkIpKernelEntriesRemoved(intfIPv4, true);
  checkIpKernelEntriesRemoved(intfIPv6, false);
}

void checkKernelIpEntriesExist(
    TunManager* tunMgr,
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& ifid,
    const std::string& intfIP,
    bool isIPv4) {
  auto status = tunMgr->getIntfStatus(state, ifid);

  if (status) {
    if (isIPv4) {
      checkKernelEntriesExist(folly::to<std::string>(intfIP), true, false);
    } else {
      checkKernelEntriesExist(folly::to<std::string>(intfIP), false, false);
    }
  }
}

void checkKernelIpEntriesRemoved(
    TunManager* tunMgr,
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& ifId,
    const std::string& intfIP,
    bool isIPv4) {
  auto status = tunMgr->getIntfStatus(state, ifId);

  if (status) {
    if (isIPv4) {
      checkKernelEntriesNotExist(folly::to<std::string>(intfIP), true, true);
    } else {
      checkKernelEntriesNotExist(folly::to<std::string>(intfIP), false, true);
    }
  }
}

void checkKernelIpEntriesRemovedStrict(
    const InterfaceID& /* ifId */,
    const std::string& intfIP,
    bool isIPv4) {
  if (isIPv4) {
    checkKernelEntriesNotExist(folly::to<std::string>(intfIP), true, true);
  } else {
    checkKernelEntriesNotExist(folly::to<std::string>(intfIP), false, true);
  }
}

std::vector<std::string> getInterfaceIpAddress(
    cfg::SwitchConfig& config,
    bool isIPv4) {
  std::string intfIP;
  std::string intfIPv4;
  std::string intfIPv6;
  std::vector<std::string> intfIPList;

  for (int i = 0; i < config.interfaces()->size(); i++) {
    if (config.interfaces()[i].scope() == cfg::Scope::GLOBAL) {
      continue;
    }
    for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
      intfIP = folly::to<std::string>(
          folly::IPAddress::createNetwork(
              config.interfaces()[i].ipAddresses()[j], -1, false)
              .first);
      if (isIPv4) {
        if (intfIP.find("::") == std::string::npos) {
          intfIPv4 = std::move(intfIP);
          intfIPList.push_back(intfIPv4);
        }
      } else {
        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
          intfIPList.push_back(intfIPv6);
        }
      }
    }
  }

  return intfIPList;
}

} // namespace facebook::fboss::utility
