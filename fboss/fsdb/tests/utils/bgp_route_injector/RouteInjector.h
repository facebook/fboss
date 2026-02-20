// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <string>
#include <vector>

namespace facebook::fboss {

struct RouteSpec {
  std::string prefix;
  std::vector<std::string> nextHops;
};

class RouteInjector {
 public:
  explicit RouteInjector(
      const std::string& agentHost = "127.0.0.1",
      uint16_t agentPort = 5909);

  void addRoute(const RouteSpec& spec);
  void addRoutes(const std::vector<RouteSpec>& specs);
  void deleteRoute(const std::string& prefix);
  void deleteAllRoutes();

  const std::map<std::string, RouteSpec>& getRoutes() const {
    return routes_;
  }

 private:
  std::string agentHost_;
  uint16_t agentPort_;
  std::map<std::string, RouteSpec> routes_;
};

} // namespace facebook::fboss
