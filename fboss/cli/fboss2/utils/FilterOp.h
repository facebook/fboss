// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {
class FilterOp {
 public:
  virtual ~FilterOp() {}
  FilterOp() {}
  virtual bool compare(const int& resultValue, const int& predicateValue) {
    return false;
  }
  virtual bool compare(const long& resultValue, const long& predicateValue) {
    return false;
  }
  virtual bool compare(const short& resultValue, const short& predicateValue) {
    return false;
  }
  virtual bool compare(
      const std::string& resultValue,
      const std::string& predicateValue) {
    return false;
  }
  virtual bool compare(
      const std::byte& resultValue,
      const std::byte& predicateValue) {
    return false;
  }
};

class FilterOpEq : public FilterOp {
 public:
  bool compare(const int& resultValue, const int& predicateValue) override {
    return resultValue == predicateValue;
  }
  bool compare(const long& resultValue, const long& predicateValue) override {
    return resultValue == predicateValue;
  }
  bool compare(const short& resultValue, const short& predicateValue) override {
    return resultValue == predicateValue;
  }
  bool compare(
      const std::string& resultValue,
      const std::string& predicateValue) override {
    return resultValue == predicateValue;
  }
  bool compare(const std::byte& resultValue, const std::byte& predicateValue)
      override {
    return resultValue == predicateValue;
  }
};

class FilterOpNeq : public FilterOp {
 public:
  bool compare(const int& resultValue, const int& predicateValue) override {
    return resultValue != predicateValue;
  }
  bool compare(const long& resultValue, const long& predicateValue) override {
    return resultValue != predicateValue;
  }
  bool compare(const short& resultValue, const short& predicateValue) override {
    return resultValue != predicateValue;
  }
  bool compare(
      const std::string& resultValue,
      const std::string& predicateValue) override {
    return resultValue != predicateValue;
  }
  bool compare(const std::byte& resultValue, const std::byte& predicateValue)
      override {
    return resultValue != predicateValue;
  }
};

class FilterOpGt : public FilterOp {
 public:
  bool compare(const int& resultValue, const int& predicateValue) override {
    return resultValue > predicateValue;
  }
  bool compare(const long& resultValue, const long& predicateValue) override {
    return resultValue > predicateValue;
  }
  bool compare(const short& resultValue, const short& predicateValue) override {
    return resultValue > predicateValue;
  }
  bool compare(
      const std::string& resultValue,
      const std::string& predicateValue) override {
    std::cerr << "> operator on strings not supported" << std::endl;
    exit(1);
    return false;
  }
  bool compare(const std::byte& resultValue, const std::byte& predicateValue)
      override {
    return resultValue > predicateValue;
  }
};

class FilterOpLt : public FilterOp {
 public:
  bool compare(const int& resultValue, const int& predicateValue) override {
    return resultValue < predicateValue;
  }
  bool compare(const long& resultValue, const long& predicateValue) override {
    return resultValue < predicateValue;
  }
  bool compare(const short& resultValue, const short& predicateValue) override {
    return resultValue < predicateValue;
  }
  bool compare(
      const std::string& resultValue,
      const std::string& predicateValue) override {
    std::cerr << "< operator on strings not supported" << std::endl;
    exit(1);
    return false;
  }
  bool compare(const std::byte& resultValue, const std::byte& predicateValue)
      override {
    return resultValue < predicateValue;
  }
};

class FilterOpGte : public FilterOp {
 public:
  bool compare(const int& resultValue, const int& predicateValue) override {
    return resultValue >= predicateValue;
  }
  bool compare(const long& resultValue, const long& predicateValue) override {
    return resultValue >= predicateValue;
  }
  bool compare(const short& resultValue, const short& predicateValue) override {
    return resultValue >= predicateValue;
  }
  bool compare(
      const std::string& resultValue,
      const std::string& predicateValue) override {
    std::cerr << ">= operator on strings not supported" << std::endl;
    exit(1);
    return false;
  }
  bool compare(const std::byte& resultValue, const std::byte& predicateValue)
      override {
    return resultValue >= predicateValue;
  }
};

class FilterOpLte : public FilterOp {
 public:
  bool compare(const int& resultValue, const int& predicateValue) override {
    return resultValue <= predicateValue;
  }
  bool compare(const long& resultValue, const long& predicateValue) override {
    return resultValue <= predicateValue;
  }
  bool compare(const short& resultValue, const short& predicateValue) override {
    return resultValue <= predicateValue;
  }
  bool compare(
      const std::string& resultValue,
      const std::string& predicateValue) override {
    std::cerr << "<= operator on strings not supported" << std::endl;
    exit(1);
    return false;
  }
  bool compare(const std::byte& resultValue, const std::byte& predicateValue)
      override {
    return resultValue <= predicateValue;
  }
};
} // namespace facebook::fboss
