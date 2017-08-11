/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "HighresCounterSubscriptionHandler.h"

#include <folly/MoveWrapper.h>

#include "fboss/agent/Utils.h"

using folly::EventBase;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::nanoseconds;
using std::chrono::seconds;

namespace facebook { namespace fboss {

// Wrapper for the actual Thrift call
inline void SampleSender::publish(unique_ptr<CounterPublication> pub) {
  if (!killSwitch_->isSet()) {
    // Note that it's okay to give the callback a shared_ptr to the client
    // without the eventBase because the actual call and callback are run in
    // the eventbase thread.
    auto& client = client_;
    auto& killSwitch = killSwitch_;
    client->publishCounters(
        [killSwitch, client](
            apache::thrift::ClientReceiveState&& state) mutable {
          // For oneway functions like this one, only exceptions make it here.
          if (state.isException()) {
            if (!killSwitch->set()) {
              LOG(ERROR) << "Exception sending publication: "
                         << state.exception();
            }
            // else, we were already dying so don't beat a dead horse
          } else {
            LOG(ERROR) << "There was a result to a oneway call";
          }
        },
        *pub);
    rateCalc_.finishedSamples(pub->times.size() * numCounters_);
  }
}

SampleProducer::SampleProducer(unique_ptr<HighresSamplerList> samplers,
                               shared_ptr<SampleSender> sender,
                               shared_ptr<Signal> killSwitch,
                               EventBase* const eventBase,
                               const CounterSubscribeRequest& req,
                               const int numCounters)
    : samplers_(std::move(samplers)),
      killSwitch_(std::move(killSwitch)),
      sender_(std::move(sender)),
      eventBase_(eventBase),
      hostname_(getLocalHostname()),
      maxTime_(seconds(req.maxTime)),
      maxCount_(req.maxCount),
      interval_(nanoseconds(req.intervalInNs)),
      batchSize_(req.batchSize),
      sleepMethod_(req.sleepMethod),
      rateCalc_("SampleProducer"),
      numCounters_(numCounters) {
  overloadWarningCounter_ = 0;
  numSamplesAtLastOverloadWarning_ = 0;
}

inline void SampleProducer::nanosleepHelper(const nanoseconds& timeLeft) {
  const int64_t kNsPerS = 1000 * 1000 * 1000;
  const auto count = timeLeft.count();

  timespec ts;
  ts.tv_sec = count / kNsPerS;
  ts.tv_nsec = count % kNsPerS;

  // Ignore signals
  if (nanosleep(&ts, nullptr)) {
    PCHECK(errno == EINTR);
  }
}

inline void SampleProducer::sleepNs(high_resolution_clock::time_point* start) {
  auto endTime = *start + interval_;
  auto currentTime = high_resolution_clock::now();
  auto timeLeft = endTime - currentTime;

  if (timeLeft < nanoseconds(0)) {
    // If processing took longer than an interval, warn of a possible overload
    if (++overloadWarningCounter_ % kOverloadWarningEveryN == 0) {
      double percent = ((double)kOverloadWarningEveryN) /
                       (numSamples_ - numSamplesAtLastOverloadWarning_) * 100;
      LOG(WARNING) << "Interval is too small for " << percent
                   << "% of samples. Exceeded by "
                   << -duration_cast<nanoseconds>(timeLeft).count() << " ns.";

      numSamplesAtLastOverloadWarning_ = numSamples_;
      overloadWarningCounter_ = 0;
    }
  } else {
    // Else, we need to sleep for a bit.  There are two ways to do it:
    if (sleepMethod_ == SleepMethod::NANOSLEEP) {
      nanosleepHelper(duration_cast<nanoseconds>(timeLeft));
      currentTime = high_resolution_clock::now();
    } else if (sleepMethod_ == SleepMethod::PAUSE) {
      do {
        // If we need to have *precise* timing, and it's not achievable with any
        // other means like 'nanosleep' or EventBase.
        asm volatile("pause");
        currentTime = high_resolution_clock::now();
      } while (currentTime < endTime);
    }
  }

  *start = currentTime;
}

inline void SampleProducer::buildPublication(
    CounterPublication* pub,
    const high_resolution_clock::time_point& currentTime) {
  auto duration = duration_cast<nanoseconds>(currentTime.time_since_epoch());

  auto time_s = duration_cast<seconds>(duration).count();
  auto time_ns = duration_cast<nanoseconds>(duration % seconds(1)).count();

  pub->times.emplace_back(
      apache::thrift::FragileConstructor::FRAGILE, time_s, time_ns);

  // Add a round of values
  for (const auto& sampler : (*samplers_.get())) {
    sampler->sample(pub);
  }
}

void SampleProducer::publish(unique_ptr<CounterPublication> pub) {
  auto& sender = sender_;
  auto wrappedPub = folly::makeMoveWrapper(std::move(pub));
  // Schedule the send in a eb thread.  We include the a shared pointer to the
  // sender so it doesn't get destroyed too early.
  eventBase_->runInEventBaseThread(
      [sender, wrappedPub]() mutable { sender->publish(wrappedPub.move()); });
}

void SampleProducer::produce() {
  auto pub = make_unique<CounterPublication>();

  pub->hostname = hostname_;
  auto batchCounter = 0;

  sender_->initialize();
  rateCalc_.initialize();
  auto currentTime = high_resolution_clock::now();
  auto timeout = currentTime + maxTime_;

  for (int i = 0;
       !killSwitch_->isSet() && i < maxCount_ && currentTime < timeout;
       ++i) {
    buildPublication(pub.get(), currentTime);

    // Check if we have a full batch.  If so move it to the queue and make a new
    // publication.
    if (++batchCounter >= batchSize_) {
      publish(std::move(pub));
      pub = make_unique<CounterPublication>();
      pub->hostname = hostname_;
      batchCounter = 0;
    }

    // Print out the sampling rate every second
    rateCalc_.finishedSamples(numCounters_);
    ++numSamples_;

    // Sleep for the remaining portion of interval
    sleepNs(&currentTime);
  }

  if (batchCounter > 0) {
    publish(std::move(pub));
  }
}
}} // facebook::fboss
