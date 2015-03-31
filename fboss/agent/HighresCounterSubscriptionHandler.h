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

#include <folly/ProducerConsumerQueue.h>

#include "fboss/agent/if/gen-cpp2/FbossHighresClient.h"
#include "fboss/agent/HighresCounterUtil.h"

namespace facebook { namespace fboss {

// How often to print out load warnings
const int kOverloadWarningEveryN = 1000;
const int kQueueFullWarningEveryN = 10000;
// How to sleep (i.e., using nanosleep() or asm ("pause"))
enum class SleepMethod { NANOSLEEP, PAUSE };
const enum SleepMethod kSleepMethod = SleepMethod::PAUSE;

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

typedef folly::Synchronized<Signal> KillSwitch;

/*
 * A single-producer-single-consumer queue to store publications before they
 * are sent.  It is implemented with a lock-free folly::ProducerConsumerQueue
 * with a few wrapper functions to add blocking reads/writes and error messages.
 */
class PublicationQueue {
 public:
  explicit PublicationQueue(uint32_t size) : queue_(size) {}

  void blockingRead(std::unique_ptr<CounterPublication>& pub) {
    while (!queue_.read(pub)) {
      sched_yield();
    }
  }

  void bestEffortWrite(std::unique_ptr<CounterPublication> pub) {
    if (!queue_.write(std::move(pub))) {
      LOG_EVERY_N(ERROR, kQueueFullWarningEveryN)
          << "PublicationQueue is full.  SampleProducer is dropping samples.";
    }
  }

  void blockingWrite(std::unique_ptr<CounterPublication> pub) {
    while (!queue_.write(std::move(pub))) {
      sched_yield();
    }
  }

 private:
  // Non-copyable
  PublicationQueue(const PublicationQueue&) = delete;
  PublicationQueue& operator=(const PublicationQueue&) = delete;

  folly::ProducerConsumerQueue<std::unique_ptr<CounterPublication>> queue_;
};

/*
 * The class that consumes publications from a shared queue and sends them
 * back to the subscribed client. When a thrift handler gets a subscription
 * request, it spawns a producer and sender and (mostly) forgets about them. The
 * sender continues until it receives a signal from the producer that there are
 * no more values (in the form of a nullptr in the queue) or a signal from the
 * thrift handler that the connection is about to be destroyed.
 */
class SampleSender {
 public:
  /*
   * Constructor.  It sets up a bunch of internal state.
   *
   * @param[in,out] queue        The MPMC queue that will hold publications.
   *                             This should be shared with the SampleProducer.
   * @param[in,out] killSwitch   A shared kill switch that lets different
   *                             threads signal errors and closed connections.
   *                             We will share this switch with thrift callbacks
   *                             so they can inform us of errors.
   * @param[in]     numCounters  The total number of counters that we are
   *                             sampling. Used for rate calculations.
   */
  SampleSender(std::shared_ptr<PublicationQueue> queue,
               std::shared_ptr<KillSwitch> killSwitch,
               const int numCounters)
      : queue_(std::move(queue)),
        killSwitch_(std::move(killSwitch)),
        rateCalc_("SampleSender"),
        numCounters_(numCounters) {}

  /*
   * Function where we actually consume publications.  Does so recursively until
   * we get a nullptr or kill signal.
   *
   * @param[in,out]  eventBase   The event base. We need this because the client
   *                             needs to be destroyed in the event base thread,
   *                             and this function runs in a separate thread.
   * @param[in,out]  client      The duplex client. We will share this with
   *                             thrift callbacks so that it doesn't get
   *                             destroyed while the calls are queued up.
   */
  void consume(apache::thrift::async::TEventBase* eventBase,
               std::shared_ptr<FbossHighresClientAsyncClient> client);

 private:
  // Non-copyable
  SampleSender(const SampleSender&) = delete;
  SampleSender& operator=(const SampleSender&) = delete;

  inline bool tryPublish(std::shared_ptr<FbossHighresClientAsyncClient>& client,
                         CounterPublication* pub);

  std::shared_ptr<PublicationQueue> queue_;
  std::shared_ptr<KillSwitch> killSwitch_;
  SingleThreadRateCalculator rateCalc_;
  const int numCounters_;
};

/*
 * The class that builds updates for the subscribed client. When a thrift
 * server gets a subscription request, it spawns a Producer and Sender and
 * (mostly) forgets about them. The producer will then continue on its own for a
 * preset amount of time or until a kill signal is received.
 */
class SampleProducer {
 public:
  /*
   * Constructor.  It pulls out information from the request and sets up a bunch
   * of internal state
   *
   * @param[in]     samplers    The entire set of samplers to query in each
   *                            round.
   * @param[in,out] queue       The MPMC queue that will hold publications. This
   *                            should be shared with the SampleSender.
   * @param[in]     killSwitch  A shared kill switch that lets different
   *                            threads signal errors and closed connections.
   * @param[in]     req         The subscription request, which specifies the
   *                            counter names, timeouts, sampling intervals,
   *                            etc.
   * @param[in]     numCounters The total number of counters that we are
   *                            sampling. Used for rate calculations.
   */
  SampleProducer(std::unique_ptr<HighresSamplerList> samplers,
                 std::shared_ptr<PublicationQueue> queue,
                 std::shared_ptr<KillSwitch> killSwitch,
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

  // Sleep until we are ready to take another sample.  We will try to sleep
  // until start + interval_.  It takes a start time so we can account for the
  // time that has already elapsed while taking samples.
  inline void sleepNs(
      const std::chrono::high_resolution_clock::time_point& start);

  // Iteratively build the publication by querying all the samplers once
  inline void buildPublication(
      CounterPublication* pub,
      const std::chrono::high_resolution_clock::time_point& time);

  // Helper classes to poll counters
  std::unique_ptr<HighresSamplerList> samplers_;

  // Variables storing information about the subscription
  const std::string hostname_;
  const std::chrono::seconds maxTime_;
  const int64_t maxCount_;
  const std::chrono::nanoseconds interval_;
  const int32_t batchSize_;

  // Shared state between the sender and producer
  std::shared_ptr<PublicationQueue> queue_;
  std::shared_ptr<KillSwitch> killSwitch_;

  // Variables to keep track of the rate at which we are processing updates
  SingleThreadRateCalculator rateCalc_;
  const int numCounters_;
  int numSamples_;
  int overloadWarningCounter_;
  int numSamplesAtLastOverloadWarning_;
};
}} // facebook::fboss
