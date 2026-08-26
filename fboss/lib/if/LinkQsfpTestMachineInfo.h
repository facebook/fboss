// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#pragma once

#include <string>

namespace facebook::fboss::utility {

// Identity of the switch a link/qsfp test is running on, stamped onto the rows
// dumped for the fboss_link_qsfp_test_port_errors Scuba table.
//
// These are Meta-internal lookups (FbWhoAmI / NetWhoAmI), so the definitions
// live in facebook/ with an oss/ stub that returns empty strings. Callers must
// tolerate an empty result: it means either an OSS build or a host that is not
// a provisioned switch.

// Device name, e.g. "rsw001.abc0.facebook.com".
std::string getMachineName();

// Hardware type of the switch, e.g. "MINIPACK3N". Empty when NetWhoAmI cannot
// be recovered (dev hosts, unprovisioned switches).
std::string getMachineType();

} // namespace facebook::fboss::utility
