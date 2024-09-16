// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/fsdb/oper/DeltaValue.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"
#include "fboss/thrift_cow/gen-cpp2/patch_types.h"

#include <boost/core/noncopyable.hpp>
#include <folly/coro/AsyncPipe.h>
#include <folly/coro/AsyncScope.h>
#include <folly/io/async/EventBase.h>
#include <folly/json/dynamic.h>

namespace facebook::fboss::fsdb {

class BaseSubscription {
 public:
  virtual PubSubType type() const = 0;

  const SubscriberId subscriberId() const {
    return subscriber_;
  }

  virtual ~BaseSubscription();

  virtual bool shouldConvertToDynamic() const {
    return false;
  }

  OperProtocol operProtocol() const {
    return protocol_;
  }

  virtual bool isActive() const = 0;

  virtual bool shouldPrune() const {
    return !isActive();
  }

  virtual void allPublishersGone(
      FsdbErrorCode disconnectReason = FsdbErrorCode::ALL_PUBLISHERS_GONE,
      const std::string& msg = "All publishers dropped") = 0;

  std::string publisherTreeRoot() const {
    // TODO - make clients deal with null publisher tree roots
    return publisherTreeRoot_ ? *publisherTreeRoot_ : "";
  }

  OperMetadata getMetadata(
      const SubscriptionMetadataServer& metadataServer) const {
    OperMetadata metadata;
    auto operTreeMetadata = metadataServer.getMetadata(publisherTreeRoot());
    if (operTreeMetadata) {
      metadata = operTreeMetadata->operMetadata;
    }
    return metadata;
  }

  virtual void flush(const SubscriptionMetadataServer& metadataServer) = 0;

  virtual void serveHeartbeat() = 0;

  folly::EventBase* heartbeatEvb() const {
    return heartbeatEvb_;
  }

  std::chrono::milliseconds heartbeatInterval() const {
    return heartbeatInterval_;
  }

 protected:
  BaseSubscription(
      SubscriberId subscriber,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval);

 private:
  folly::coro::Task<void> heartbeatLoop();

  const SubscriberId subscriber_;
  const OperProtocol protocol_;
  const std::optional<std::string> publisherTreeRoot_;
  folly::EventBase* heartbeatEvb_;
  folly::coro::CancellableAsyncScope backgroundScope_;
  std::chrono::milliseconds heartbeatInterval_;
};

class Subscription : public BaseSubscription {
 public:
  const std::vector<std::string>& path() const {
    return path_;
  }

  bool needsFirstChunk() const {
    return needsFirstChunk_;
  }

  void firstChunkSent() {
    needsFirstChunk_ = false;
  }

 protected:
  Subscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval)
      : BaseSubscription(
            std::move(subscriber),
            std::move(protocol),
            std::move(publisherRoot),
            heartbeatEvb,
            std::move(heartbeatInterval)),
        path_(std::move(path)) {}

 private:
  const std::vector<std::string> path_;
  bool needsFirstChunk_{true};
};

using ExtSubPathMap = std::map<SubscriptionKey, ExtendedOperPath>;

class ExtendedSubscription : public BaseSubscription {
 public:
  const ExtSubPathMap& paths() const {
    return paths_;
  }

  std::size_t size() const {
    return paths_.size();
  }

  bool shouldPrune() const override {
    return shouldPrune_;
  }

  bool markShouldPruneIfInactive() {
    if (!isActive()) {
      shouldPrune_ = true;
    }
    return shouldPrune_;
  }

  const ExtendedOperPath& pathAt(SubscriptionKey key) const {
    return paths_.at(key);
  }

  virtual std::unique_ptr<Subscription> resolve(
      const SubscriptionKey& key,
      const std::vector<std::string>& path) = 0;

 protected:
  ExtendedSubscription(
      SubscriberId subscriber,
      ExtSubPathMap paths,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval)
      : BaseSubscription(
            std::move(subscriber),
            std::move(protocol),
            std::move(publisherRoot),
            std::move(heartbeatEvb),
            std::move(heartbeatInterval)),
        paths_(std::move(paths)) {}

 private:
  const ExtSubPathMap paths_;
  bool shouldPrune_{false};
};

class BasePathSubscription : public Subscription {
 public:
  virtual ~BasePathSubscription() override = default;

  using Subscription::Subscription;

  virtual void offer(DeltaValue<OperState> newVal) = 0;

  PubSubType type() const override {
    return PubSubType::PATH;
  }
};

class PathSubscription : public BasePathSubscription,
                         private boost::noncopyable {
 public:
  using value_type = DeltaValue<OperState>;

  virtual ~PathSubscription() override = default;
  using PathIter = std::vector<std::string>::const_iterator;

  void offer(DeltaValue<OperState> newVal) override {
    // use bounded pipe?
    pipe_.write(std::move(newVal));
  }

  bool isActive() const override {
    return !pipe_.isClosed();
  }

  static std::pair<
      folly::coro::AsyncGenerator<value_type&&>,
      std::unique_ptr<PathSubscription>>
  create(
      SubscriberId subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval) {
    auto [generator, pipe] = folly::coro::AsyncPipe<value_type>::create();
    std::vector<std::string> path(begin, end);
    auto subscription = std::make_unique<PathSubscription>(
        std::move(subscriber),
        std::move(path),
        std::move(pipe),
        std::move(protocol),
        std::move(publisherRoot),
        std::move(heartbeatEvb),
        std::move(heartbeatInterval));
    return std::make_pair(std::move(generator), std::move(subscription));
  }

  bool shouldConvertToDynamic() const override {
    return false;
  }

  void allPublishersGone(FsdbErrorCode disconnectReason, const std::string& msg)
      override {
    pipe_.write(Utils::createFsdbException(disconnectReason, msg));
  }

  void flush(const SubscriptionMetadataServer& /*metadataServer*/) override {
    // no-op, we write directly to the pipe in offer
  }

  void serveHeartbeat() override {
    value_type t;
    t.newVal = OperState();
    // need to explicitly set a flag for OperState else it looks like a deletion
    t.newVal->isHeartbeat() = true;
    pipe_.write(std::move(t));
  }

  PathSubscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      folly::coro::AsyncPipe<value_type> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval)
      : BasePathSubscription(
            std::move(subscriber),
            std::move(path),
            std::move(protocol),
            std::move(publisherTreeRoot),
            std::move(heartbeatEvb),
            std::move(heartbeatInterval)),
        pipe_(std::move(pipe)) {}

 private:
  folly::coro::AsyncPipe<value_type> pipe_;
};

class BaseDeltaSubscription : public Subscription {
 public:
  virtual ~BaseDeltaSubscription() override = default;

  PubSubType type() const override {
    return PubSubType::DELTA;
  }

  bool shouldConvertToDynamic() const override {
    return false;
  }

  void appendRootDeltaUnit(const OperDeltaUnit& unit);

  BaseDeltaSubscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval)
      : Subscription(
            std::move(subscriber),
            std::move(path),
            std::move(protocol),
            std::move(publisherTreeRoot),
            heartbeatEvb,
            std::move(heartbeatInterval)) {}

  std::optional<OperDelta> moveFromCurrDelta(
      const SubscriptionMetadataServer& metadataServer);

 private:
  std::optional<OperDelta> currDelta_;
};

class DeltaSubscription : public BaseDeltaSubscription,
                          private boost::noncopyable {
 public:
  using PathIter = std::vector<std::string>::const_iterator;

  void flush(const SubscriptionMetadataServer& metadataServer) override;

  bool isActive() const override;

  void allPublishersGone(FsdbErrorCode disconnectReason, const std::string& msg)
      override;

  static std::pair<
      folly::coro::AsyncGenerator<OperDelta&&>,
      std::unique_ptr<DeltaSubscription>>
  create(
      SubscriberId subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval);

  DeltaSubscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      folly::coro::AsyncPipe<OperDelta> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval)
      : BaseDeltaSubscription(
            std::move(subscriber),
            std::move(path),
            std::move(protocol),
            std::move(publisherTreeRoot),
            heartbeatEvb,
            std::move(heartbeatInterval)),
        pipe_(std::move(pipe)) {}

  void serveHeartbeat() override;

 private:
  folly::coro::AsyncPipe<OperDelta> pipe_;
};

class ExtendedPathSubscription;

class FullyResolvedExtendedPathSubscription : public BasePathSubscription,
                                              private boost::noncopyable {
 public:
  virtual ~FullyResolvedExtendedPathSubscription() override = default;

  FullyResolvedExtendedPathSubscription(
      const std::vector<std::string>& path,
      ExtendedPathSubscription& subscription);

  PubSubType type() const override;

  bool isActive() const override;
  bool shouldPrune() const override;

  void offer(DeltaValue<OperState> newVal) override;

  void allPublishersGone(FsdbErrorCode disconnectReason, const std::string& msg)
      override;

  void flush(const SubscriptionMetadataServer& metadataServer) override;

  void serveHeartbeat() override;

 private:
  ExtendedPathSubscription& subscription_;
};

class ExtendedPathSubscription : public ExtendedSubscription,
                                 private boost::noncopyable {
 public:
  virtual ~ExtendedPathSubscription() override = default;
  // we only support encoded extended subscriptions
  using value_type = TaggedOperState;
  using gen_type = std::vector<DeltaValue<value_type>>;

  void buffer(DeltaValue<value_type>&& newVal);

  void flush(const SubscriptionMetadataServer& metadataServer) override;

  void serveHeartbeat() override {
    pipe_.write(gen_type());
  }

  PubSubType type() const override {
    return PubSubType::PATH;
  }

  virtual std::unique_ptr<Subscription> resolve(
      const SubscriptionKey& key,
      const std::vector<std::string>& path) override;

  static std::pair<
      folly::coro::AsyncGenerator<gen_type&&>,
      std::shared_ptr<ExtendedPathSubscription>>
  create(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      std::optional<std::string> publisherRoot,
      OperProtocol protocol,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval);

  bool shouldConvertToDynamic() const override {
    return false;
  }

  bool isActive() const override;

  void allPublishersGone(FsdbErrorCode disconnectReason, const std::string& msg)
      override;

  ExtendedPathSubscription(
      SubscriberId subscriber,
      ExtSubPathMap paths,
      folly::coro::AsyncPipe<gen_type> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval)
      : ExtendedSubscription(
            std::move(subscriber),
            std::move(paths),
            std::move(protocol),
            std::move(publisherTreeRoot),
            std::move(heartbeatEvb),
            std::move(heartbeatInterval)),
        pipe_(std::move(pipe)) {}

 private:
  folly::coro::AsyncPipe<gen_type> pipe_;
  std::optional<gen_type> buffered_;
};

class ExtendedDeltaSubscription;

class FullyResolvedExtendedDeltaSubscription : public BaseDeltaSubscription,
                                               private boost::noncopyable {
 public:
  virtual ~FullyResolvedExtendedDeltaSubscription() override = default;

  FullyResolvedExtendedDeltaSubscription(
      const std::vector<std::string>& path,
      ExtendedDeltaSubscription& subscription);

  void flush(const SubscriptionMetadataServer& metadataServer) override;

  void serveHeartbeat() override;

  PubSubType type() const override;

  bool isActive() const override;
  bool shouldPrune() const override;

  void allPublishersGone(FsdbErrorCode disconnectReason, const std::string& msg)
      override;

 private:
  ExtendedDeltaSubscription& subscription_;
};

class ExtendedDeltaSubscription : public ExtendedSubscription,
                                  private boost::noncopyable {
 public:
  virtual ~ExtendedDeltaSubscription() override = default;
  // we only support encoded extended subscriptions
  using value_type = std::vector<TaggedOperDelta>;

  // we flatten the delta lists in to one list, so just use the same type
  using gen_type = value_type;

  PubSubType type() const override {
    return PubSubType::DELTA;
  }

  virtual std::unique_ptr<Subscription> resolve(
      const SubscriptionKey& key,
      const std::vector<std::string>& path) override;

  static std::pair<
      folly::coro::AsyncGenerator<gen_type&&>,
      std::shared_ptr<ExtendedDeltaSubscription>>
  create(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      std::optional<std::string> publisherRoot,
      OperProtocol protocol,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval);

  void buffer(TaggedOperDelta&& newVal);

  void flush(const SubscriptionMetadataServer& metadataServer) override;

  void serveHeartbeat() override;

  bool shouldConvertToDynamic() const override {
    return false;
  }

  bool isActive() const override;

  void allPublishersGone(FsdbErrorCode disconnectReason, const std::string& msg)
      override;

  ExtendedDeltaSubscription(
      SubscriberId subscriber,
      ExtSubPathMap paths,
      folly::coro::AsyncPipe<value_type> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval)
      : ExtendedSubscription(
            std::move(subscriber),
            std::move(paths),
            std::move(protocol),
            std::move(publisherTreeRoot),
            std::move(heartbeatEvb),
            std::move(heartbeatInterval)),
        pipe_(std::move(pipe)) {}

 private:
  folly::coro::AsyncPipe<gen_type> pipe_;
  std::optional<gen_type> buffered_;
};

class ExtendedPatchSubscription;
class PatchSubscription : public Subscription, private boost::noncopyable {
 public:
  using PathIter = std::vector<std::string>::const_iterator;

  PatchSubscription(
      const SubscriptionKey& key,
      std::vector<std::string> path,
      ExtendedPatchSubscription& subscription);

  virtual ~PatchSubscription() override = default;

  PubSubType type() const override {
    return PubSubType::PATCH;
  }

  void offer(thrift_cow::PatchNode node);

  void serveHeartbeat() override;

  void flush(const SubscriptionMetadataServer& metadataServer) override;

  bool isActive() const override;

  virtual void allPublishersGone(
      FsdbErrorCode disconnectReason = FsdbErrorCode::ALL_PUBLISHERS_GONE,
      const std::string& msg = "All publishers dropped") override;

 private:
  SubscriptionKey key_;
  std::optional<Patch> currPatch_;
  ExtendedPatchSubscription& subscription_;
};

class ExtendedPatchSubscription : public ExtendedSubscription,
                                  private boost::noncopyable {
 public:
  using gen_type = SubscriberMessage;

  virtual ~ExtendedPatchSubscription() override = default;

  // Single path
  static std::pair<
      folly::coro::AsyncGenerator<gen_type&&>,
      std::unique_ptr<ExtendedPatchSubscription>>
  create(
      SubscriberId subscriber,
      std::vector<std::string> path,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval);

  // Multipath
  static std::pair<
      folly::coro::AsyncGenerator<gen_type&&>,
      std::unique_ptr<ExtendedPatchSubscription>>
  create(
      SubscriberId subscriber,
      std::map<SubscriptionKey, RawOperPath> paths,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval);

  // Extended paths
  static std::pair<
      folly::coro::AsyncGenerator<gen_type&&>,
      std::unique_ptr<ExtendedPatchSubscription>>
  create(
      SubscriberId subscriber,
      ExtSubPathMap paths,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval);

  ExtendedPatchSubscription(
      SubscriberId subscriber,
      ExtSubPathMap paths,
      folly::coro::AsyncPipe<gen_type> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot,
      folly::EventBase* heartbeatEvb,
      std::chrono::milliseconds heartbeatInterval)
      : ExtendedSubscription(
            std::move(subscriber),
            std::move(paths),
            std::move(protocol),
            std::move(publisherTreeRoot),
            std::move(heartbeatEvb),
            std::move(heartbeatInterval)),
        pipe_(std::move(pipe)) {}

  PubSubType type() const override {
    return PubSubType::PATCH;
  }

  virtual std::unique_ptr<Subscription> resolve(
      const SubscriptionKey& key,
      const std::vector<std::string>& path) override;

  void buffer(const SubscriptionKey& key, Patch&& newVal);

  void flush(const SubscriptionMetadataServer& metadataServer) override;

  void serveHeartbeat() override;

  bool isActive() const override;

  void allPublishersGone(FsdbErrorCode disconnectReason, const std::string& msg)
      override;

 private:
  std::optional<SubscriberChunk> moveCurChunk(
      const SubscriptionMetadataServer& metadataServer);

  std::map<SubscriptionKey, std::vector<Patch>> buffered_;
  folly::coro::AsyncPipe<gen_type> pipe_;
};

} // namespace facebook::fboss::fsdb
