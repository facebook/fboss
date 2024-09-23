// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <iostream>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <folly/init/Init.h>
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_constants.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"

using namespace facebook::fboss::fsdb;

auto fsdbClient() {
  auto params = facebook::servicerouter::ClientParams();
  params.setSingleHost("::1", fsdb_common_constants::PORT());
  params.setProcessingTimeoutMs(std::chrono::milliseconds(500));

  return facebook::servicerouter::cpp2::getClientFactory()
      .getSRClientUnique<FsdbServiceAsyncClient>("", params);
}

folly::coro::Task<void> subscribe(std::vector<std::string> rawPath) {
  auto client = fsdbClient();
  OperSubRequest req;
  OperPath path;
  path.raw() = rawPath;
  req.path() = path;
  req.protocol() = OperProtocol::SIMPLE_JSON;

  auto gen = (co_await client->co_subscribeOperStatePath(req))
                 .stream.toAsyncGenerator();
  while (auto val = co_await gen.next()) {
    // got value
    std::cout << "New value: " << *val->contents() << std::endl;
  }
  co_return;
}

folly::coro::Task<void> subscribeDelta(std::vector<std::string> rawPath) {
  auto client = fsdbClient();
  OperSubRequest req;
  OperPath path;
  path.raw() = rawPath;
  req.path() = path;
  req.protocol() = OperProtocol::SIMPLE_JSON;

  auto gen = (co_await client->co_subscribeOperStateDelta(req))
                 .stream.toAsyncGenerator();
  while (auto val = co_await gen.next()) {
    // got value
    std::cout << "New Delta: " << std::endl;
    for (auto& change : *val->changes()) {
      if (change.newState()) {
        std::cout << "\t/" << folly::join("/", *change.path()->raw()) << ": "
                  << *change.newState() << std::endl;
      } else {
        std::cout << "\tDeleted /" << folly::join("/", *change.path()->raw())
                  << std::endl;
      }
    }
  }
  co_return;
}

folly::coro::Task<void> subscribeExtended(std::vector<ExtendedOperPath> paths) {
  auto client = fsdbClient();
  OperSubRequestExtended req;
  req.paths() = paths;
  req.protocol() = OperProtocol::SIMPLE_JSON;
  req.subscriberId() = "test";

  int num{0};
  auto gen = (co_await client->co_subscribeOperStatePathExtended(req))
                 .stream.toAsyncGenerator();
  while (auto val = co_await gen.next()) {
    // got value
    std::cout << "Received batch " << num++ << std::endl;
    for (auto& state : *val->changes()) {
      std::cout << "Path=" << folly::join('#', *state.path()->path())
                << std::endl;
      auto contents =
          (state.state()->contents()) ? *state.state()->contents() : "null";
      std::cout << "Contents=" << contents << std::endl << std::endl;
    }
  }
  co_return;
}

folly::coro::Task<void> subscribeDeltaExtended(
    std::vector<ExtendedOperPath> paths) {
  auto client = fsdbClient();
  OperSubRequestExtended req;
  req.paths() = paths;
  req.protocol() = OperProtocol::SIMPLE_JSON;
  req.subscriberId() = "test";

  int num{0};
  auto gen = (co_await client->co_subscribeOperStateDeltaExtended(req))
                 .stream.toAsyncGenerator();
  while (auto val = co_await gen.next()) {
    // got value
    std::cout << "Received batch " << num++ << std::endl;
    for (auto& delta : *val->changes()) {
      std::cout << "Path: " << folly::join("/", *delta.path()->path());
      for (auto& change : *delta.delta()->changes()) {
        if (change.newState()) {
          std::cout << "\t/" << folly::join("/", *change.path()->raw()) << ": "
                    << *change.newState() << std::endl;
        } else {
          std::cout << "\tDeleted /" << folly::join("/", *change.path()->raw())
                    << std::endl;
        }
      }
    }
  }
  co_return;
}

ExtendedOperPath parseExtendedOperPath(
    const std::vector<std::string>& rawPath) {
  ExtendedOperPath extendedPath;
  for (auto& tok : rawPath) {
    OperPathElem elem;
    if (tok.size() > 2 && tok[0] == '/' && tok[tok.size() - 1] == '/') {
      elem.regex_ref() = tok.substr(1, tok.size() - 2);
    } else if (tok == ".*") {
      elem.any_ref() = true;
    } else {
      elem.raw_ref() = tok;
    }
    extendedPath.path()->emplace_back(std::move(elem));
  }
  return extendedPath;
}

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  if (argc < 3) {
    std::cout << "Incorrect usage. Expected 2 arguments, function and path"
              << std::endl;
    return 1;
  }

  try {
    std::string function(argv[1]);
    std::string pathStr(argv[2]);

    auto client = fsdbClient();

    std::vector<std::string> rawPath;
    folly::split('#', pathStr, rawPath);

    if (function == "get") {
      OperGetRequest req;
      req.path()->raw() = rawPath;
      req.protocol() = OperProtocol::SIMPLE_JSON;

      auto result =
          folly::coro::blockingWait(client->co_getOperState(std::move(req)));
      auto contents = (result.contents()) ? *result.contents() : "null";
      std::cout << "Success!! Result=" << contents << std::endl;
      if (auto meta = result.metadata(); meta) {
        std::cout << "Last confirmed at=" << meta->lastConfirmedAt().value_or(0)
                  << std::endl;
      }
    } else if (function == "set") {
      if (argc < 4) {
        std::cout << "Incorrect usage. Expected 3 arguments, function, "
                     "path and value"
                  << std::endl;
        return 1;
      }

      std::string contents = argv[3];
      std::string filePrefix = "file:";
      if (contents.size() > filePrefix.size()) {
        auto res = std::mismatch(
            filePrefix.begin(), filePrefix.end(), contents.begin());
        auto path =
            std::string(contents.begin() + filePrefix.size(), contents.end());
        if (res.first == filePrefix.end()) {
          folly::readFile(path.c_str(), contents);
        }
      }
      OperPubRequest req;
      OperPath path;
      req.path()->raw() = rawPath;
      req.publisherId() = "test";

      auto result =
          folly::coro::blockingWait(client->co_publishOperStatePath(req));

      std::cout << "Started successfully" << std::endl;

      folly::coro::blockingWait(
          result.sink.sink([&]() -> folly::coro::AsyncGenerator<OperState&&> {
            OperState chunk;
            chunk.contents() = contents;
            chunk.protocol() = OperProtocol::SIMPLE_JSON;
            co_yield std::move(chunk);
          }()));
      std::cout << "Success!!" << std::endl;
    } else if (function == "subscribe") {
      folly::coro::blockingWait(subscribe(std::move(rawPath)));
    } else if (function == "subscribe_delta") {
      folly::coro::blockingWait(subscribeDelta(std::move(rawPath)));
    } else if (function == "subscribe_extended") {
      std::vector<ExtendedOperPath> paths;
      paths.emplace_back(parseExtendedOperPath(rawPath));

      for (int i = 3; i < argc; ++i) {
        std::string nextPath(argv[i]);
        std::vector<std::string> nextPathToks;
        folly::split('#', nextPath, nextPathToks);
        paths.emplace_back(parseExtendedOperPath(nextPathToks));
      }

      folly::coro::blockingWait(subscribeExtended(std::move(paths)));
    } else if (function == "subscribe_delta_extended") {
      std::vector<ExtendedOperPath> paths;
      paths.emplace_back(parseExtendedOperPath(rawPath));

      for (int i = 3; i < argc; ++i) {
        std::string nextPath(argv[i]);
        std::vector<std::string> nextPathToks;
        folly::split('#', nextPath, nextPathToks);
        paths.emplace_back(parseExtendedOperPath(nextPathToks));
      }

      folly::coro::blockingWait(subscribeDeltaExtended(std::move(paths)));
    } else if (function == "get_extended") {
      OperGetRequestExtended req;
      req.paths()->emplace_back(parseExtendedOperPath(rawPath));
      req.protocol() = OperProtocol::SIMPLE_JSON;

      auto result = folly::coro::blockingWait(
          client->co_getOperStateExtended(std::move(req)));
      for (auto& state : result) {
        std::cout << "Path=" << folly::join('#', *state.path()->path())
                  << std::endl;
        auto contents =
            (state.state()->contents()) ? *state.state()->contents() : "null";
        std::cout << "Contents=" << contents << std::endl << std::endl;
      }
    } else {
      std::cout << "Incorrect usage. Choose from 'get', 'get_extended', "
                << "'subscribe', 'subscribe_delta' or 'set'" << std::endl;
      return 1;
    }
  } catch (const FsdbException& e) {
    std::cerr << "ec=" << apache::thrift::util::enumName(*e.errorCode_ref())
              << ", error=" << *e.message_ref() << std::endl;
    return 5;
  }
}
