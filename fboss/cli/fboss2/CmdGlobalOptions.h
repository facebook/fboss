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

#include <CLI/CLI.hpp>
#include "fboss/cli/fboss2/options/SSLPolicy.h"

namespace facebook::fboss {

class CmdGlobalOptions {
 public:
  CmdGlobalOptions() = default;
  ~CmdGlobalOptions() = default;
  CmdGlobalOptions(const CmdGlobalOptions& other) = delete;
  CmdGlobalOptions& operator=(const CmdGlobalOptions& other) = delete;

  // Static function for getting the CmdGlobalOptions folly::Singleton
  static std::shared_ptr<CmdGlobalOptions> getInstance();

  void init(CLI::App& app);

  bool isValid() const {
    bool hostsSet = !getHosts().empty();
    bool smcSet = !getSmc().empty();
    bool fileSet = !getFile().empty();

    if ((hostsSet && (smcSet || fileSet)) ||
        (smcSet && (hostsSet || fileSet)) ||
        (fileSet && (hostsSet || smcSet))) {
      std::cerr << "only one of host(s), smc or file can be set\n";
      return false;
    }

    return true;
  }

  std::vector<std::string> getHosts() const {
    return hosts_;
  }

  std::string getSmc() const {
    return smc_;
  }

  std::string getFile() const {
    return file_;
  }

  std::string getLogLevel() const {
    return logLevel_;
  }

  const SSLPolicy& getSslPolicy() const {
    return sslPolicy_;
  }

  std::string getFmt() const {
    return fmt_;
  }

  std::string getLogUsage() const {
    return logUsage_;
  }

  int getAgentThriftPort() const {
    return agentThriftPort_;
  }

  int getQsfpThriftPort() const {
    return qsfpThriftPort_;
  }

  int getBgpThriftPort() const {
    return bgpThriftPort_;
  }

  int getMkaThriftPort() const {
    return mkaThriftPort_;
  }
  int getCoopThriftPort() const {
    return coopThriftPort_;
  }

  int getRackmonThriftPort() const {
    return rackmonThriftPort_;
  }

  int getBmcHttpPort() const {
    return bmcHttpPort_;
  }

  std::string getColor() const {
    return color_;
  }

  // Setters for testing purposes
  void setAgentThriftPort(int port) {
    agentThriftPort_ = port;
  }

  void setBgpThriftPort(int port) {
    bgpThriftPort_ = port;
  }

 private:
  void initAdditional(CLI::App& app);

  std::vector<std::string> hosts_;
  std::string smc_;
  std::string file_;
  std::string logLevel_{"DBG0"};
  SSLPolicy sslPolicy_{"plaintext"};
  std::string fmt_{"tabular"};
  std::string logUsage_{"scuba"};
  int agentThriftPort_{5909};
  int qsfpThriftPort_{5910};
  int bgpThriftPort_{6909};
  int coopThriftPort_{6969};
  int mkaThriftPort_{5920};
  int bmcHttpPort_{8080};
  int rackmonThriftPort_{7910};
  std::string color_{"yes"};
};

} // namespace facebook::fboss
