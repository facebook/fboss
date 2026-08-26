/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/CmdShowBgpInitializationEvents.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <iostream>
#include <string>

#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

CmdShowBgpInitializationEvents::RetType
CmdShowBgpInitializationEvents::queryClient(const HostInfo& hostInfo) {
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  // Get initialization events and their timestamps
  RetType events;
  client->sync_getInitializationEvents(events);

  return events;
}

void CmdShowBgpInitializationEvents::printOutput(
    const RetType& events,
    std::ostream& out) {
  using facebook::fboss::utils::Table;

  // prevent printing number with comma, e.g: 65,000
  out.imbue(std::locale("C"));

  // Check if BGP has converged based on the presence of INITIALIZED event
  bool converged =
      events.find(
          neteng::fboss::bgp::thrift::BgpInitializationEvent::INITIALIZED) !=
      events.end();
  out << "BGP Initialization Converged: " << (converged ? "Yes" : "No")
      << std::endl
      << std::endl;

  Table table;
  table.setHeader({"Event", "Status", "Time From Start"});

  // Track which events actually occurred (have timestamps)
  std::set<neteng::fboss::bgp::thrift::BgpInitializationEvent> completedEvents;
  int32_t highestCompletedEvent = -1;
  for (const auto& [event, timestamp] : events) {
    if (timestamp > 0) {
      completedEvents.insert(event);
      int32_t eventValue = static_cast<int32_t>(event);
      if (eventValue > highestCompletedEvent) {
        highestCompletedEvent = eventValue;
      }
    }
  }

  // Get all enum values dynamically
  const auto& enumValues = apache::thrift::TEnumTraits<
      neteng::fboss::bgp::thrift::BgpInitializationEvent>::values;

  // Add rows to the table for each event
  for (const auto& enumValue : enumValues) {
    auto event =
        static_cast<neteng::fboss::bgp::thrift::BgpInitializationEvent>(
            enumValue);
    std::string eventName = apache::thrift::util::enumNameSafe(event);
    std::string status;
    if (completedEvents.count(event)) {
      status = "Complete";
    } else if (converged) {
      // BGP has converged, so this event was skipped in this initialization
      // path
      status = "Skipped";
    } else if (static_cast<int32_t>(enumValue) < highestCompletedEvent) {
      // BGP has progressed past this event, so it was skipped
      status = "Skipped";
    } else {
      // BGP is still initializing and hasn't reached this event yet
      status = "Pending";
    }

    std::string duration = "-";

    if (events.count(event) && events.at(event) > 0) {
      // Use BGP service timestamp directly (already represents time from
      // start)
      int64_t durationMs = events.at(event);

      // Format duration with millisecond precision
      if (durationMs == 0) {
        duration = "0ms";
      } else {
        int64_t totalSeconds = durationMs / 1000;
        int64_t milliseconds = durationMs % 1000;

        int64_t hours = totalSeconds / 3600;
        int64_t minutes = (totalSeconds % 3600) / 60;
        int64_t seconds = totalSeconds % 60;

        if (hours > 0) {
          duration = fmt::format(
              "{}h {}m {}s {}ms", hours, minutes, seconds, milliseconds);
        } else if (minutes > 0) {
          duration =
              fmt::format("{}m {}s {}ms", minutes, seconds, milliseconds);
        } else if (seconds > 0) {
          duration = fmt::format("{}s {}ms", seconds, milliseconds);
        } else {
          duration = fmt::format("{}ms", milliseconds);
        }
      }
    }

    table.addRow({eventName, status, duration});
  }

  out << table << std::endl;
}

template void CmdHandler<
    CmdShowBgpInitializationEvents,
    CmdShowBgpInitializationEventsTraits>::run();

} // namespace facebook::fboss
