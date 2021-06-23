/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <string>
#include "fboss/cli/fboss2/tabulate/table.hpp"

namespace facebook::fboss::utils {

/*
  A wrapper around our tabulation library. This is the only place we should be
  using tabulate, to make it easier to swap libraries if required.
*/
class Table {
 public:
  class Row {
   public:
    explicit Row(tabulate::Row& row) : internalRow_(row) {}

   private:
    friend class Table;
    tabulate::Row& internalRow_;
  };

  Table();

  Row& setHeader(const std::vector<std::string>& data);
  Row& addRow(const std::vector<std::string>& data);

 private:
  friend std::ostream& operator<<(std::ostream& stream, const Table& table);

  std::vector<Row> rows_;
  tabulate::Table internalTable_;
};

inline std::ostream& operator<<(std::ostream& stream, const Table& table) {
  return stream << table.internalTable_;
}

} // namespace facebook::fboss::utils
