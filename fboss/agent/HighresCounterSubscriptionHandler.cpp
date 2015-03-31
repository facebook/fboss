/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
 #include "fboss/agent/HighresCounterSubscriptionHandler.h"

using apache::thrift::async::TEventBase;
using apache::thrift::HandlerCallback;
using folly::make_unique;
using std::istringstream;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::seconds;

namespace facebook { namespace fboss {

// Primary function to consume samples from the queue and send them to the
// client.
void SampleSender::consume(
    TEventBase* eventBase,
    std::shared_ptr<FbossHighresClientAsyncClient> client) {
  rateCalc_.initialize();
  bool finished = false;

  do {
    // It's okay that this blocks and connectionDestroyed waits on this method.
    // Even if this occurs, the Producer will die, queue a nullptr, and we will
    // see it.
    unique_ptr<CounterPublication> pub;
    queue_->blockingRead(pub);
    if (pub) {
      if (tryPublish(client, pub.get())) {
        rateCalc_.finishedSamples(pub->times.size() * numCounters_);
      } else {
        finished = true;
      }
    } else {
      finished = true;
    }
  } while (!finished);

  // Client expects to be destroyed in the event base thread.  Make sure that
  // happens.
  auto wrappedClient = folly::makeMoveWrapper(std::move(client));
  eventBase->runInEventBaseThread(
      [wrappedClient]() mutable { (*wrappedClient).reset(); });
}

// Wrapper for the actual Thrift call
inline bool SampleSender::tryPublish(
    shared_ptr<FbossHighresClientAsyncClient>& client,
    CounterPublication* pub) {
  auto killSwitch = killSwitch_;
  auto& sync_ks = *(killSwitch.get());
  bool published = false;
  SYNCHRONIZED_CONST(sync_ks) {
    if (!sync_ks.isSet()) {
      // Note that it's okay to give the callback a shared_ptr to the client
      // without the eventBase because the actual call and callback are run in
      // the eventbase thread.
      client->publishCounters(
          [killSwitch, client](
              apache::thrift::ClientReceiveState&& state) mutable {
            // For oneway functions like this one, only exceptions make it here.
            if (state.isException()) {
              if (!(*killSwitch)->set()) {
                LOG(ERROR) << "Exception sending publication: "
                           << folly::exceptionStr(state.exception());
              }
              // else, we were already dying so don't beat a dead horse
            } else {
              LOG(ERROR) << "There was a result to a oneway call";
            }
          },
          *pub);
      published = true;
    }
  }
  return published;
}

// Helper function to get the current machine's hostname
string getLocalHostname() {
  const size_t kHostnameMaxLen = 256;  // from gethostname man page
  char hostname[kHostnameMaxLen];
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    LOG(ERROR) << "gethostname() failed";
    return "";
  }

  return hostname;
}

SampleProducer::SampleProducer(unique_ptr<HighresSamplerList> samplers,
                               shared_ptr<PublicationQueue> queue,
                               shared_ptr<KillSwitch> killSwitch,
                               const CounterSubscribeRequest& req,
                               const int numCounters)
    : samplers_(std::move(samplers)),
      hostname_(getLocalHostname()),
      maxTime_(seconds(req.maxTime)),
      maxCount_(req.maxCount),
      interval_(nanoseconds(req.intervalInNs)),
      batchSize_(req.batchSize),
      queue_(std::move(queue)),
      killSwitch_(std::move(killSwitch)),
      rateCalc_("SampleProducer"),
      numCounters_(numCounters) {
  overloadWarningCounter_ = 0;
  numSamplesAtLastOverloadWarning_ = 0;
}

/*
 * Helper function to create a timespec from a std::chrono::duaration object
 */
inline timespec convertToTimespec(const nanoseconds& n) {
  const int64_t k_ns_per_s = 1000 * 1000 * 1000;
  timespec ts;

  ts.tv_sec = n.count() / k_ns_per_s;
  ts.tv_nsec = n.count() % k_ns_per_s;

  return ts;
}

inline void SampleProducer::sleepNs(
    const high_resolution_clock::time_point& start) {
  auto end = start + interval_;
  auto now = high_resolution_clock::now();
  auto timeLeft = end - now;

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
    if (kSleepMethod == SleepMethod::NANOSLEEP) {
      timespec ts = convertToTimespec(duration_cast<nanoseconds>(timeLeft));
      // Ignore signals
      if (nanosleep(&ts, nullptr)) {
        PCHECK(errno == EINTR);
      }
    } else if (kSleepMethod == SleepMethod::PAUSE) {
      do {
        // If we need to have *precise* timing, and it's not achievable with any
        // other means like 'nanosleep' or EventBase.
        asm volatile("pause");
      } while (high_resolution_clock::now() < end);
    }
  }
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

void SampleProducer::produce() {
  auto pub = make_unique<CounterPublication>();

  pub->hostname = hostname_;
  auto batchCounter = 0;

  rateCalc_.initialize();
  auto currentTime = high_resolution_clock::now();
  auto timeout = currentTime + maxTime_;

  for (int i = 0; !killSwitch_->asConst()->isSet() && i < maxCount_ &&
                      currentTime < timeout;
       ++i) {
    buildPublication(pub.get(), currentTime);

    // Check if we have a full batch.  If so move it to the queue and make a new
    // publication.
    if (++batchCounter >= batchSize_) {
      queue_->bestEffortWrite(std::move(pub));
      pub = make_unique<CounterPublication>();
      pub->hostname = hostname_;
      batchCounter = 0;
    }

    // Print out the sampling rate every second
    rateCalc_.finishedSamples(numCounters_);
    ++numSamples_;

    // Sleep for the remaining portion of interval
    sleepNs(currentTime);

    currentTime = high_resolution_clock::now();
  }

  if (batchCounter > 0) {
    queue_->bestEffortWrite(std::move(pub));
  }

  // Add a sentinel "null" value
  queue_->blockingWrite(nullptr);
}
}} // facebook::fboss
