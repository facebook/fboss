namespace cpp2 facebook.fboss.cli

struct ShowTransceiverModel {
  1: map<i32, TransceiverDetail> transceivers;
}

struct TransceiverDetail {
  1: string name;
  2: bool isUp;
  3: bool isPresent;
}
