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

DEFINE_bool(print_rates,
            false,
            "Whether to enable RateCalculator calculations and printouts.");

namespace facebook { namespace fboss {

void DumbCounterSampler::sample(CounterPublication* pub) {
  pub->counters[kDumbCounterFullName].push_back(++counter_);
}

InterfaceRateSampler::InterfaceRateSampler(
    const std::set<folly::StringPiece>& counters) {
  for (const auto& c : counters) {
    if (c.compare(kTxBytesCounterName) == 0 ||
        c.compare(kRxBytesCounterName) == 0) {
      counters_.insert(c.toString());
    }
  }

  struct stat buffer;

  if (!counters_.empty() && (stat("/proc/net/dev", &buffer) == 0)) {
    numCounters_ = counters_.size();
  } else {
    numCounters_ = 0;
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

  if (counters_.count(kTxBytesCounterName)) {
    pub->counters[kTxBytesCounterFullName].push_back(sout);
  }
  if (counters_.count(kRxBytesCounterName)) {
    pub->counters[kRxBytesCounterFullName].push_back(sin);
  }
}
}} // facebook::fboss
