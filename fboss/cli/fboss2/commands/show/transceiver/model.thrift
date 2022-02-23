namespace cpp2 facebook.fboss.cli

struct ShowTransceiverModel {
  1: map<i32, TransceiverDetail> transceivers;
}

struct TransceiverDetail {
  1: string name;
  2: bool isUp;
  3: bool isPresent;
  4: string vendor;
  5: string serial;
  6: string partNumber;
  7: double temperature;
  8: double voltage;
  9: list<double> currentMA;
  10: list<double> txPower;
  11: list<double> rxPower;
  12: list<double> rxSnr;
}
