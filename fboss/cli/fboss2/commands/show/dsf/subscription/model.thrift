namespace cpp2 facebook.fboss.cli

struct ShowDsfSubscriptionModel {
  1: list<Subscription> subscriptions;
}

struct Subscription {
  1: string name;
  2: string state;
  3: list<string> paths;
}
