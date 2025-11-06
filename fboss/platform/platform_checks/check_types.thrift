package "meta.com/fboss/platform/platform_checks"

namespace cpp2 facebook.fboss.platform.platform_checks

/*
 * Identifies the type of check
 */
enum CheckType {
  PCI_DEVICE_CHECK = 1,
  MAC_ADDRESS_CHECK = 2,
  RECENT_MANUAL_REBOOT_CHECK = 3,
  RECENT_KERNEL_PANIC_CHECK = 4,
  WATCHDOG_DID_NOT_STOP_CHECK = 5,
  KERNEL_VERSION_CHECK = 6,
}

/*
 * OK - check passed
 * PROBLEM - check failed in an expected way. Remediation may be available.
 * ERROR - unable to complete the check properly
 */
enum CheckStatus {
  OK = 1,
  PROBLEM = 2,
  ERROR = 3,
}

/*
 * Identifies the remediation required
 *
 * MANUAL_REMEDIATION - further information is reported in the CheckResult
 */
enum RemediationType {
  NONE = 1,
  REBOOT_REQUIRED = 2,
  REPROVISION_REQUIRED = 3,
  RMA_REQUIRED = 4,
  MANUAL_REMEDIATION = 5,
}

struct CheckResult {
  1: CheckType checkType;
  2: CheckStatus status;
  3: optional RemediationType remediation;
  4: optional string remediationMessage;
  5: optional string errorMessage;
  6: optional string checkName;
}

struct CheckInfo {
  1: CheckType checkType;
  2: set<string> supportedPlatforms;
  3: string description;
}

struct CheckReport {
  1: list<CheckResult> results;
}
