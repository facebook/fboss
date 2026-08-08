// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <compare>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::platform_manager {
namespace package_manager {

// Parsed BSP version (major.minor.patch). Ordered, so callers can gate behavior
// on a threshold version. fromString() fails closed (std::nullopt) when the
// input is not a parseable x.y.z triple.
struct BspVersion {
  int major{};
  int minor{};
  int patch{};

  static std::optional<BspVersion> fromString(std::string_view version);

  auto operator<=>(const BspVersion&) const = default;
};

// Outcome of an RPM install attempt. notFound distinguishes "no such package in
// any enabled repo" -- a permanent failure, usually a wrong version in the
// platform_manager config -- from transient failures worth retrying.
struct RpmInstallResult {
  int exitStatus{0};
  bool notFound{false};
};

class SystemInterface {
 public:
  explicit SystemInterface(
      const std::shared_ptr<PlatformUtils>& platformUtils =
          std::make_shared<PlatformUtils>());
  virtual ~SystemInterface() = default;
  virtual bool loadKmod(const std::string& moduleName) const;
  virtual bool unloadKmod(const std::string& moduleName) const;
  virtual RpmInstallResult installRpm(
      const std::string& rpmFullName,
      const std::string& repoName = "") const;
  virtual int depmod() const;
  virtual std::vector<std::string> getInstalledRpms(
      const std::string& rpmBaseName) const;
  virtual int removeRpms(const std::vector<std::string>& installedRpms) const;
  virtual std::set<std::string> lsmod() const;
  virtual bool isRpmInstalled(const std::string& rpmFullName) const;
  virtual std::string getHostKernelVersion() const;
  int installLocalRpm() const;

  // Returns the BSP version installed for the running kernel: from the RPMs
  // named rpmBaseName, selects the one whose package name matches
  // getHostKernelVersion() and parses out its version token; std::nullopt if
  // none matches or the token is unparseable. Non-virtual: derived purely from
  // the virtual host-state reads above, so tests drive it by faking
  // getHostKernelVersion() and getInstalledRpms().
  std::optional<BspVersion> getInstalledBspVersion(
      const std::string& rpmBaseName) const;

  // Returns the {version}-{release} tokens of the BSP kmods RPM files sitting
  // in the local RPM repo (--local_rpm_repo_path) whose package name targets
  // the running kernel, newest first. Empty when the repo directory is absent
  // or holds nothing for this kernel.
  virtual std::vector<std::string> getLocalRepoBspRpmVersions(
      const std::string& rpmBaseName) const;

 private:
  std::shared_ptr<PlatformUtils> platformUtils_;
};

} // namespace package_manager
} // namespace facebook::fboss::platform::platform_manager
