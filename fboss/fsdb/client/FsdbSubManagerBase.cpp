// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbSubManagerBase.h"

namespace facebook::fboss::fsdb {

FsdbSubManagerBase::FsdbSubManagerBase(
    fsdb::SubscriptionOptions opts,
    utils::ConnectionOptions serverOptions,
    folly::EventBase* reconnectEvb,
    folly::EventBase* subscriberEvb)
    : opts_(std::move(opts)),
      connectionOptions_(std::move(serverOptions)),
      reconnectEvbThread_(
          reconnectEvb ? nullptr
                       : std::make_unique<folly::ScopedEventBaseThread>(
                             kReconnectThread)),
      subscriberEvbThread_(
          subscriberEvb ? nullptr
                        : std::make_unique<folly::ScopedEventBaseThread>(
                              kSubscriberThread)),
      reconnectEvb_(
          reconnectEvbThread_ ? reconnectEvbThread_->getEventBase()
                              : reconnectEvb),
      subscriberEvb_(
          subscriberEvbThread_ ? subscriberEvbThread_->getEventBase()
                               : subscriberEvb) {}

FsdbSubManagerBase::~FsdbSubManagerBase() {
  stop();
}

void FsdbSubManagerBase::stop() {
  subscriber_.reset();
}

const std::string& FsdbSubManagerBase::clientId() const {
  return opts_.clientId_;
}

std::optional<SubscriptionInfo> FsdbSubManagerBase::getInfo() {
  if (subscriber_) {
    return subscriber_->getInfo();
  }
  return std::nullopt;
}

SubscriptionKey FsdbSubManagerBase::addPathImpl(
    const std::vector<std::string>& pathTokens) {
  CHECK(!subscriber_) << "Cannot add paths after subscribed";
  auto key = nextKey_++;
  RawOperPath p;
  p.path() = pathTokens;
  auto res = subscribePaths_.insert_or_assign(key, std::move(p));
  CHECK(res.second) << "Duplicate path added";
  return key;
}

void FsdbSubManagerBase::subscribeImpl(
    std::function<void(SubscriberChunk&&)> chunkHandler,
    std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb) {
  CHECK(!subscriber_) << "Cannot subscribe twice";
  subscriber_ = std::make_unique<FsdbPatchSubscriber>(
      SubscriptionOptions(opts_),
      subscribePaths_,
      subscriberEvb_,
      reconnectEvb_,
      std::move(chunkHandler),
      std::move(subscriptionStateChangeCb));
  subscriber_->setConnectionOptions(connectionOptions_);
}

} // namespace facebook::fboss::fsdb
