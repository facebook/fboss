/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HighresCounterUtil.h"

#include <sys/stat.h>

#include <folly/logging/xlog.h>
#include <sstream>

DEFINE_bool(print_rates,
            false,
            "Whether to enable RateCalculator calculations and printouts.");

namespace facebook { namespace fboss {

DumbCounterSampler::DumbCounterSampler(
    const std::set<CounterRequest>& counters) {
  for (const auto& c: counters) {
    if (c.counterName == kDumbCounterName) {
      // If there are duplicate requests, overwrite
      req_ = c;
      numCounters_ = 1;
    } else {
      XLOG(WARNING) << "Requested counter " << c.counterName
                    << " does not exist";
    }
  }
}

void DumbCounterSampler::sample(CounterPublication* pub) {
  pub->counterValues[req_].push_back(++counter_);
}

InterfaceRateSampler::InterfaceRateSampler(
    const std::set<CounterRequest>& counters) {
  struct stat buffer;
  if (stat("/proc/net/dev", &buffer) != 0) {
    XLOG(ERR) << "/proc/net/dev does not exist."
              << " Ignoring any InterfaceRateSamplers";
    return;
  }

  for (const auto& c : counters) {
    if (c.counterName == kTxBytesCounterName ||
        c.counterName == kRxBytesCounterName) {
      counters_.insert(c);
    }
  }
}

void InterfaceRateSampler::sample(CounterPublication* pub) {
  uint64_t sin = -1;
  uint64_t sout = -1;

  ifs_.open("/proc/net/dev");
  std::string line, buf;
  if (ifs_.is_open()) {
    // In/out traffic in bytes are located at the 1st and the 9th tokens
    // for each interface. Indexes start at 0.
    // We consider only ethN interfaces.
    sin = sout = 0;
    while (getline(ifs_, line)) {
      std::istringstream iss(line);

      iss >> buf;
      const std::string prefix = "eth";

      if (buf.substr(0, prefix.size()) == prefix) {
        iss >> buf;
        sin += atoll(buf.c_str());

        for (int tokid = 2; tokid < 10; ++tokid)
          iss >> buf;
        sout += atoll(buf.c_str());
      }
    }
    ifs_.close();
  }

  for (const auto& c : counters_) {
    if (c.counterName == kTxBytesCounterName) {
      pub->counterValues[c].push_back(sout);
    } else if (c.counterName == kRxBytesCounterName) {
      pub->counterValues[c].push_back(sin);
    }
  }
}
}} // facebook::fboss
