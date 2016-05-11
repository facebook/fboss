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

#include <folly/MoveWrapper.h>

#include "fboss/agent/HighresCounterUtil.h"
#include "fboss/agent/if/gen-cpp2/FbossHighresClient.h"

namespace facebook { namespace fboss {

// How often to print out load warnings
const int kOverloadWarningEveryN = 1000;

/*
 * A simple, atomic flag that starts unset, but can later be set.  Wrapped in a
 * folly::Synchronized wrapper and shared_ptr, it becomes an atomic, thread-safe
 * kill switch.
 */
class Signal {
 public:
  Signal() : s_(false) {}
  inline bool set() { return s_.exchange(true, std::memory_order_release); }
  inline bool isSet() const { return s_.load(std::memory_order_acquire); }

 private:
  // Non-copyable
  Signal(const Signal&) = delete;
  Signal& operator=(const Signal&) = delete;

  std::atomic<bool> s_;
};

/*
 * A helper class that sends publications back to the subscribed client.
 */
class SampleSender {
 public:
  /*
   * Constructor.  It sets up a bunch of internal state.
   *
   * @param[in]     client       The duplex client. We will share this with
   *                             thrift callbacks so that it doesn't get
   *                             destroyed while the calls are queued up.
   * @param[in,out] killSwitch   A shared kill switch that lets different
   *                             threads signal errors and closed connections.
   *                             We will share this switch with thrift callbacks
   *                             so they can inform us of errors.
   * @param[in,out] eventBase    The event base. We need this because the client
   *                             needs to be destroyed in the event base thread.
   * @param[in]     numCounters  The total number of counters that we are
   *                             sampling. Used for rate calculations.
   */
  SampleSender(std::shared_ptr<FbossHighresClientAsyncClient> client,
               std::shared_ptr<Signal> killSwitch,
               folly::EventBase* const eventBase,
               const int numCounters)
      : client_(std::move(client)),
        killSwitch_(std::move(killSwitch)),
        eventBase_(eventBase),
        rateCalc_("SampleSender"),
        numCounters_(numCounters) {}

  /*
   * Destructor that ensures that the client is destroyed in the event base
   * thread.
   */
  ~SampleSender() {
    auto wrappedClient = folly::makeMoveWrapper(std::move(client_));
    eventBase_->runInEventBaseThread(
        [wrappedClient]() mutable { (*wrappedClient).reset(); });
  }

  /*
   * Initializes the rate calculator
   */
  void initialize() { rateCalc_.initialize(); }

  /*
   * Publish the counters back to the duplex client.  This should be called from
   * the event base thread where client_ came from.
   *
   * @param[in]   pub    The finished publication to send. It will be destroyed
   *                     after this function returns.
   */
  void publish(std::unique_ptr<CounterPublication> pub);

 private:
  // Non-copyable
  SampleSender(const SampleSender&) = delete;
  SampleSender& operator=(const SampleSender&) = delete;

  std::shared_ptr<FbossHighresClientAsyncClient> client_;
  std::shared_ptr<Signal> killSwitch_;
  folly::EventBase* const eventBase_;
  SharedRateCalculator rateCalc_;
  const int numCounters_;
};

/*
 * The class that builds updates for the subscribed client. When a thrift
 * server gets a subscription request, it spawns a Producer and (mostly) forgets
 * about it. The producer will then continue on its own for a preset amount of
 * time or until a kill signal is received.
 */
class SampleProducer {
 public:
  /*
   * Constructor.  It pulls out information from the request and sets up a bunch
   * of internal state
   *
   * @param[in]     samplers    The entire set of samplers to query in each
   *                            round.
   * @param[out]    sender      The SampleSender that we call to publish data
   *                            back to the client.
   * @param[in]     killSwitch  A shared kill switch that lets different
   *                            threads signal errors and closed connections.
   * @param[in,out] eventBase   The event base. We use it to schedule publish
   *                            calls.
   * @param[in]     req         The subscription request, which specifies the
   *                            counter names, timeouts, sampling intervals,
   *                            etc.
   * @param[in]     numCounters The total number of counters that we are
   *                            sampling. Used for rate calculations.
   */
  SampleProducer(std::unique_ptr<HighresSamplerList> samplers,
                 std::shared_ptr<SampleSender> sender,
                 std::shared_ptr<Signal> killSwitch,
                 folly::EventBase* const eventBase,
                 const CounterSubscribeRequest& req,
                 const int numCounters);

  /*
   * Continue producing until we get a kill signal or for a fixed amount of
   * time/samples, whichever comes first.
   */
  void produce();

 private:
  // Non-copyable
  SampleProducer(const SampleProducer&) = delete;
  SampleProducer& operator=(const SampleProducer&) = delete;

  // A wrapper for nanosleep that takes std::chrono::nanoseconds
  inline void nanosleepHelper(const std::chrono::nanoseconds& timeLeft);

  // Sleep until we are ready to take another sample.  We will try to sleep
  // until start + interval_.  It takes a start time so we can account for the
  // time that has already elapsed while taking samples.  After this function
  // executes, start >= start + interval.
  inline void sleepNs(std::chrono::high_resolution_clock::time_point* start);

  // Iteratively build the publication by querying all the samplers once
  inline void buildPublication(
      CounterPublication* pub,
      const std::chrono::high_resolution_clock::time_point& time);

  // Schedule the SampleSender in a tm thread
  inline void publish(std::unique_ptr<CounterPublication> pub);

  // For the normal polling loop
  std::unique_ptr<HighresSamplerList> samplers_;
  std::shared_ptr<Signal> killSwitch_;

  // For sending the publications
  std::shared_ptr<SampleSender> sender_;
  folly::EventBase* const eventBase_;

  // For storing information about the subscription
  const std::string hostname_;
  const std::chrono::seconds maxTime_;
  const int64_t maxCount_;
  const std::chrono::nanoseconds interval_;
  const int32_t batchSize_;
  const SleepMethod sleepMethod_;

  // For keeping track of the rate at which we are processing updates
  SingleThreadRateCalculator rateCalc_;
  const int numCounters_;
  int numSamples_;
  int overloadWarningCounter_;
  int numSamplesAtLastOverloadWarning_;
};
}} // facebook::fboss
