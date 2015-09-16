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

#include <folly/Synchronized.h>

#include "fboss/agent/if/gen-cpp2/highres_types.h"

#include <chrono>
#include <fstream>
#include <unordered_set>

DECLARE_bool(print_rates);

namespace facebook { namespace fboss {

class HighresSampler;
typedef std::vector<std::unique_ptr<HighresSampler>> HighresSamplerList;

/*
 * The interface expected from samplers.  Each sampler can handle multiple
 * counters, but a single sampler object should be contained within a single
 * namespace.
 */
class HighresSampler {
 public:
  virtual ~HighresSampler() {}

  /*
   * Virtual sample function. This is the function that is called by the server
   * in a loop.  It should add the the requested set of counter values to pub.
   *
   * @param[out]   pub    The publication to which we should add counter values.
   */
  virtual void sample(CounterPublication* pub) = 0;

  /*
   * The number of counters handled by this sampler.  Potential reasons why a
   * counter is invalid are if the namespace is not valid on our hardware or if
   * the counter does not exist.  There must be at least 1 counter for this
   * sampler to be valid.
   *
   * @return    The number of valid counters handled by this sampler
   */
  virtual int numCounters() const = 0;
};

/*
 * A sampler that simply increments and returns a counter.  Used for performance
 * testing/debugging.
 */
class DumbCounterSampler : public HighresSampler {
 public:
  explicit DumbCounterSampler(const std::set<folly::StringPiece>& counters)
      : counter_(0) {
    numCounters_ = counters.count(kDumbCounterName);
  }
  ~DumbCounterSampler() override {}
  void sample(CounterPublication* pub) override;
  int numCounters() const override {return numCounters_;}

  /// constant strings representing the namespace and counter names.  We store
  /// everything explicitly for speed.
  static constexpr const char* const kIdentifier = "dumb_counter";
  static constexpr const char* const kDumbCounterName = "foo";
  static constexpr const char* const kDumbCounterFullName = "dumb_counter::foo";

 private:
  int counter_;
  int numCounters_;
};

/*
 * A sampler that polls the proc file system for interface Tx/Rx rates.
 */
class InterfaceRateSampler : public HighresSampler {
 public:
  explicit InterfaceRateSampler(const std::set<folly::StringPiece>& counters);
  ~InterfaceRateSampler() override {}
  void sample(CounterPublication* pub) override;

  int numCounters() const override { return numCounters_; }

  /// constant strings representing the namespace and counter names.  We store
  /// everything explicitly for speed.
  static constexpr const char* const kIdentifier = "interface_rate";
  static constexpr const char* const kTxBytesCounterName = "tx";
  static constexpr const char* const kTxBytesCounterFullName =
      "interface_rate::tx";
  static constexpr const char* const kRxBytesCounterName = "rx";
  static constexpr const char* const kRxBytesCounterFullName =
      "interface_rate::rx";

 private:
  // We save an ifstream for reading counters without needing to repeatedly call
  // the constructor
  std::ifstream ifs_;
  std::unordered_set<std::string> counters_;

  int numCounters_;
};

/*
 * A helper class that can calculate the rate at which some entity is processing
 * samples.  The rate is calculated every ~1 second.
 */
class RateCalculatorIf {
 public:
  /*
   * Constructor.  name parameter diffrentiates this rate updater from others.
   *
   * @param[in]  name  The name of what we are tracking.
   */
  explicit RateCalculatorIf(const std::string& name) : name_(name) {}

  virtual ~RateCalculatorIf() {}
  /*
   * Should reset the number of samples to 0 and the time of the last update to
   * now. Call once soon before the start of update handling.
   */
  inline virtual void initialize() = 0;

  /*
   * Adds a batch of n samples and if a second has passed, print out statistics.
   *
   * @param[in]    n     The number of samples we have just processed.
   */
  inline virtual void finishedSamples(const int n) = 0;

  inline virtual uint_fast64_t getNumSamples() const = 0;

 protected:
  // Container class for the number of samples and time when we last printed
  // statistics. These are used to calculate the rate at which we've processed
  // updates since the last printout.
  struct LastRateCalculation {
    uint_fast64_t numSamples_;
    std::chrono::high_resolution_clock::time_point time_;
  };

  const std::string name_;
};

/*
 * Implements the RateCalculator interface for single-threaded users.
 * Implementation is here because we want to inline the functions.
 */
class SingleThreadRateCalculator : RateCalculatorIf {
 public:
  explicit SingleThreadRateCalculator(const std::string& name)
      : RateCalculatorIf(name) {}

  inline void initialize() override {
    numSamples_ = 0;
    lastRateCalc_.numSamples_ = 0;
    lastRateCalc_.time_ = std::chrono::high_resolution_clock::now();
  }

  inline void finishedSamples(const int n) override {
    if (!FLAGS_print_rates) {
      return;
    }

    // Increment by n
    numSamples_ += n;
    auto time = std::chrono::high_resolution_clock::now();
    auto& timeAtLastUpdate = lastRateCalc_.time_;

    // If a second has passed since the last rate update...
    if (time - timeAtLastUpdate > std::chrono::seconds(1)) {
      // ...then print the update and reset everything!
      LOG(INFO) << name_ << ": processing at a rate of "
                << numSamples_ - lastRateCalc_.numSamples_ << " per second";
      lastRateCalc_.numSamples_ = numSamples_;
      lastRateCalc_.time_ = time;
    }
  }

  inline uint_fast64_t getNumSamples() const override { return numSamples_; }

 private:
  uint_fast64_t numSamples_;
  LastRateCalculation lastRateCalc_;
};

/*
 * Implements the RateCalculator interface for multi-threaded users.
 * Implementation is here because we want to inline the functions.
 */
class SharedRateCalculator : RateCalculatorIf {
 public:
  explicit SharedRateCalculator(const std::string& name)
      : RateCalculatorIf(name) {}

  inline void initialize() override {
    numSamples_ = 0;
    lastRateCalc_->numSamples_ = 0;
    lastRateCalc_->time_ = std::chrono::high_resolution_clock::now();
  }

  inline void finishedSamples(const int n) override {
    if (!FLAGS_print_rates) {
      return;
    }

    // Increment by n
    numSamples_.fetch_add(n, std::memory_order_relaxed);
    auto time = std::chrono::high_resolution_clock::now();

    auto timeAtLastUpdate = lastRateCalc_.asConst()->time_;

    // If a second has passed since the last rate update...
    if (time - timeAtLastUpdate > std::chrono::seconds(1)) {
      SYNCHRONIZED(lastRateCalc_) {
        // ...and we get the write lock first...
        if (timeAtLastUpdate == lastRateCalc_.time_) {
          // ...then print the update and reset everything!
          auto tmp = getNumSamples();
          LOG(INFO) << name_ << ": processing at a rate of "
                    << tmp - lastRateCalc_.numSamples_ << " per second";
          lastRateCalc_.numSamples_ = tmp;
          lastRateCalc_.time_ = time;
        }
      }
    }
  }

  inline uint_fast64_t getNumSamples() const override {
    return numSamples_.load(std::memory_order_relaxed);
  }

 private:
  std::atomic<uint_fast64_t> numSamples_;
  folly::Synchronized<LastRateCalculation> lastRateCalc_;
};

}} // facebook::fboss
