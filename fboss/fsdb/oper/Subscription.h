// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <any>

#include <boost/core/noncopyable.hpp>

#include <fboss/thrift_cow/gen-cpp2/patch_types.h>
#include <fboss/thrift_cow/visitors/ThriftTCType.h>
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/oper/DeltaValue.h"
#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"

#include <folly/experimental/coro/AsyncPipe.h>
#include <folly/json/dynamic.h>

namespace facebook::fboss::fsdb {

class BaseSubscription {
 public:
  virtual PubSubType type() const = 0;

  const SubscriberId subscriberId() const {
    return subscriber_;
  }

  virtual ~BaseSubscription() = default;

  virtual bool shouldConvertToDynamic() const {
    return false;
  }

  virtual std::optional<OperProtocol> operProtocol() const {
    return std::nullopt;
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

 protected:
  BaseSubscription(
      SubscriberId subscriber,
      std::optional<std::string> publisherRoot)
      : subscriber_(std::move(subscriber)),
        publisherTreeRoot_(std::move(publisherRoot)) {}

 private:
  const SubscriberId subscriber_;
  const std::optional<std::string> publisherTreeRoot_;
};

class Subscription : public BaseSubscription {
 public:
  const std::vector<std::string>& path() const {
    return path_;
  }

 protected:
  Subscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      std::optional<std::string> publisherRoot)
      : BaseSubscription(std::move(subscriber), std::move(publisherRoot)),
        path_(std::move(path)) {}

 private:
  const std::vector<std::string> path_;
};

class ExtendedSubscription : public BaseSubscription {
 public:
  const std::vector<ExtendedOperPath>& paths() const {
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

  const ExtendedOperPath& pathAt(std::size_t idx) const {
    return paths_.at(idx);
  }

  virtual std::unique_ptr<Subscription> resolve(
      const std::vector<std::string>& path) = 0;

 protected:
  ExtendedSubscription(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      std::optional<std::string> publisherRoot)
      : BaseSubscription(std::move(subscriber), std::move(publisherRoot)),
        paths_(std::move(paths)) {}

 private:
  const std::vector<ExtendedOperPath> paths_;
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

  std::optional<OperProtocol> operProtocol() const override {
    return protocol_;
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
      std::optional<std::string> publisherRoot,
      std::optional<OperProtocol> protocol) {
    auto [generator, pipe] = folly::coro::AsyncPipe<value_type>::create();
    std::vector<std::string> path(begin, end);
    auto subscription = std::make_unique<PathSubscription>(
        std::move(subscriber),
        std::move(path),
        std::move(pipe),
        std::move(protocol),
        std::move(publisherRoot));
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
      std::optional<OperProtocol> protocol = std::nullopt,
      std::optional<std::string> publisherTreeRoot = std::nullopt)
      : BasePathSubscription(
            std::move(subscriber),
            std::move(path),
            std::move(publisherTreeRoot)),
        protocol_(std::move(protocol)),
        pipe_(std::move(pipe)) {}

  PathSubscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      folly::coro::AsyncPipe<value_type> pipe,
      std::optional<std::string> publisherTreeRoot = std::nullopt)
      : PathSubscription(
            std::move(subscriber),
            std::move(path),
            std::move(pipe),
            std::nullopt,
            std::move(publisherTreeRoot)) {}

 private:
  std::optional<OperProtocol> protocol_;
  folly::coro::AsyncPipe<value_type> pipe_;
};

class BaseDeltaSubscription : public Subscription {
 public:
  virtual ~BaseDeltaSubscription() override = default;

  PubSubType type() const override {
    return PubSubType::DELTA;
  }

  std::optional<OperProtocol> operProtocol() const override {
    return protocol_;
  }

  bool shouldConvertToDynamic() const override {
    return false;
  }

  void appendRootDeltaUnit(const OperDeltaUnit& unit);

  BaseDeltaSubscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot = std::nullopt)
      : Subscription(
            std::move(subscriber),
            std::move(path),
            std::move(publisherTreeRoot)),
        protocol_(std::move(protocol)) {}

  std::optional<OperDelta> moveFromCurrDelta(
      const SubscriptionMetadataServer& metadataServer);

 private:
  std::optional<OperProtocol> protocol_;
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
      std::optional<std::string> publisherRoot);

  DeltaSubscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      folly::coro::AsyncPipe<OperDelta> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot = std::nullopt)
      : BaseDeltaSubscription(
            std::move(subscriber),
            std::move(path),
            std::move(protocol),
            std::move(publisherTreeRoot)),
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

  std::optional<OperProtocol> operProtocol() const override;

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

  std::optional<OperProtocol> operProtocol() const override {
    return protocol_;
  }

  virtual std::unique_ptr<Subscription> resolve(
      const std::vector<std::string>& path) override;

  static std::pair<
      folly::coro::AsyncGenerator<gen_type&&>,
      std::shared_ptr<ExtendedPathSubscription>>
  create(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      std::optional<std::string> publisherRoot,
      OperProtocol protocol);

  bool shouldConvertToDynamic() const override {
    return false;
  }

  bool isActive() const override;

  void allPublishersGone(FsdbErrorCode disconnectReason, const std::string& msg)
      override;

  ExtendedPathSubscription(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      folly::coro::AsyncPipe<gen_type> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot = std::nullopt)
      : ExtendedSubscription(
            std::move(subscriber),
            std::move(paths),
            std::move(publisherTreeRoot)),
        protocol_(std::move(protocol)),
        pipe_(std::move(pipe)) {}

 private:
  OperProtocol protocol_;
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

  std::optional<OperProtocol> operProtocol() const override;

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

  std::optional<OperProtocol> operProtocol() const override {
    return protocol_;
  }

  virtual std::unique_ptr<Subscription> resolve(
      const std::vector<std::string>& path) override;

  static std::pair<
      folly::coro::AsyncGenerator<gen_type&&>,
      std::shared_ptr<ExtendedDeltaSubscription>>
  create(
      SubscriberId subscriber,
      std::vector<ExtendedOperPath> paths,
      std::optional<std::string> publisherRoot,
      OperProtocol protocol);

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
      std::vector<ExtendedOperPath> paths,
      folly::coro::AsyncPipe<value_type> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot = std::nullopt)
      : ExtendedSubscription(
            std::move(subscriber),
            std::move(paths),
            std::move(publisherTreeRoot)),
        protocol_(std::move(protocol)),
        pipe_(std::move(pipe)) {}

 private:
  OperProtocol protocol_;
  folly::coro::AsyncPipe<gen_type> pipe_;
  std::optional<gen_type> buffered_;
};

class PatchSubscription : public Subscription, private boost::noncopyable {
 public:
  using PathIter = std::vector<std::string>::const_iterator;

  static std::pair<
      folly::coro::AsyncGenerator<thrift_cow::Patch&&>,
      std::unique_ptr<PatchSubscription>>
  create(
      SubscriberId subscriber,
      PathIter begin,
      PathIter end,
      OperProtocol protocol,
      std::optional<std::string> publisherRoot) {
    auto [generator, pipe] =
        folly::coro::AsyncPipe<thrift_cow::Patch>::create();
    std::vector<std::string> path(begin, end);
    auto subscription = std::make_unique<PatchSubscription>(
        std::move(subscriber),
        std::move(path),
        std::move(pipe),
        std::move(protocol),
        std::move(publisherRoot));
    return std::make_pair(std::move(generator), std::move(subscription));
  }

  PatchSubscription(
      SubscriberId subscriber,
      std::vector<std::string> path,
      folly::coro::AsyncPipe<thrift_cow::Patch> pipe,
      OperProtocol protocol,
      std::optional<std::string> publisherTreeRoot = std::nullopt)
      : Subscription(
            std::move(subscriber),
            std::move(path),
            std::move(publisherTreeRoot)),
        protocol_(std::move(protocol)),
        pipe_(std::move(pipe)) {}

  virtual ~PatchSubscription() override = default;

  PubSubType type() const override {
    return PubSubType::PATCH;
  }

  std::optional<OperProtocol> operProtocol() const override {
    return protocol_;
  }

  std::optional<thrift_cow::Patch> moveFromCurrPatch(
      const SubscriptionMetadataServer& /* metadataServer */);

  void serveHeartbeat() override;

  void flush(const SubscriptionMetadataServer& /*metadataServer*/) override;

  bool isActive() const override {
    return !pipe_.isClosed();
  }

  virtual void allPublishersGone(
      FsdbErrorCode disconnectReason = FsdbErrorCode::ALL_PUBLISHERS_GONE,
      const std::string& msg = "All publishers dropped") override;

 private:
  OperProtocol protocol_;
  std::optional<thrift_cow::Patch> currPatch_;
  folly::coro::AsyncPipe<thrift_cow::Patch> pipe_;
};

} // namespace facebook::fboss::fsdb
