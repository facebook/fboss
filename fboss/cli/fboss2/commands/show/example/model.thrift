package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

/*
  The overall model we want to represent. Most commonly this will encapsulate a list of items
*/
struct ShowExampleModel {
  1: list<ExampleData> exampleData;
}

struct ExampleData {
  1: i32 id;
  2: string name;
  // More data
}
