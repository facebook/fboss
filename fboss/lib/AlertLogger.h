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

#include <folly/Format.h>
#include <folly/logging/xlog.h>

#include <ostream>
#include <string>

namespace facebook::fboss {

/*
 * Utilities for logging of hi-signal events
 * with event-type and additional metadata params over XLOG
 *
 * example invocations:
 *  XLOG(ERR) << PlatformAlert() << logMsg;
 *
 *  XLOG(INFO) << PortAlert() << logMsg <<  PortParam(port->getName());
 *
 * example outputs:
 *  ... FBOSS_ALERT(PLATFORM): Exception talking to qsfp_ service: Channel is
 * !good()
 *  ... FBOSS_ALERT(SERVICE): SwitchRunState changed from UNINITIALIZED to
 * INITIALIZED
 *  ... FBOSS_ALERT(PORT): LinkState: Port eth1/31/1 Up <port:eth1/31/1>
 */

// Alert tag with type
struct AlertTag {
  explicit AlertTag(std::string type);

  friend std::ostream& operator<<(std::ostream& out, const AlertTag& tag) {
    return (out << tag.fullTag_);
  }

  const std::string& tagString(void) {
    return fullTag_;
  }

  const std::string type_;
  const std::string fullTag_;
};

class MiscAlert : public AlertTag {
 public:
  MiscAlert();
};
class AsicAlert : public AlertTag {
 public:
  AsicAlert();
};
class ServiceAlert : public AlertTag {
 public:
  ServiceAlert();
};
class PlatformAlert : public AlertTag {
 public:
  PlatformAlert();
};
class BmcAlert : public AlertTag {
 public:
  BmcAlert();
};
class KernelAlert : public AlertTag {
 public:
  KernelAlert();
};
class PortAlert : public AlertTag {
 public:
  PortAlert();
};
class RouteAlert : public AlertTag {
 public:
  RouteAlert();
};

// Alert parameter types
struct AlertParam {
  explicit AlertParam(std::string type, std::string value);

  friend std::ostream& operator<<(std::ostream& out, const AlertParam& param) {
    return (out << " <" << param.type_ << ":" << param.value_ << ">");
  }

  const std::string type_;
  const std::string value_;
};

class PortParam : public AlertParam {
 public:
  explicit PortParam(std::string value);
};
class VlanParam : public AlertParam {
 public:
  explicit VlanParam(std::string value);
};
class Ipv4Param : public AlertParam {
 public:
  explicit Ipv4Param(std::string value);
};
class Ipv6Param : public AlertParam {
 public:
  explicit Ipv6Param(std::string value);
};
class MacParam : public AlertParam {
 public:
  explicit MacParam(std::string value);
};

} // namespace facebook::fboss
