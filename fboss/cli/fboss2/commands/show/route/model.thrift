namespace cpp2 facebook.fboss.cli

struct ShowRouteModel {
  1: list<RouteEntry> routeEntries;
}

struct RouteEntry {
  1: string ip;
  2: i32 prefixLength;
}
