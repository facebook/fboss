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
  enum Style {
    NONE,
    GOOD,
    WARN,
    ERROR,
    INFO,
  };

  class Row {
   public:
    explicit Row(tabulate::Row& row) : internalRow_(row) {}

    void setStyle(Style style);

    void setCellStyle(int cellIndex, Style style);

    friend class Table;
    tabulate::Row& internalRow_;
  };

  class StyledCell {
   public:
    /* implicit */ StyledCell(const std::string& data)
        : data_(data), style_(Style::NONE) {}
    StyledCell(const std::string& data, Style style)
        : data_(data), style_(style) {}
    StyledCell(StyledCell const& cell)
        : data_(std::string(cell.data_)), style_(cell.style_) {}

    StyledCell& operator=(StyledCell& other) = default;

    static StyledCell errorIfPos(int val) {
      if (val > 1) {
        return StyledCell{std::to_string(val), ERROR};
      }
      return StyledCell{std::to_string(val)};
    }

    std::string getData() const {
      return data_;
    }

    Style getStyle() const {
      return style_;
    }

   private:
    std::string data_;
    Style style_;
  };

  Table();

  Table(bool showBorders);

  using RowData = std::variant<std::string, StyledCell>;

  /*
    Add data to table. RowData can be in the form of a string, or a StyledCell:
    table.addRow({"val1", StyledCell("val2", Style::WARN), "val3"});
  */
  Row& setHeader(const std::vector<RowData>& data);
  Row& addRow(
      const std::vector<RowData>& data,
      Table::Style rowStyle = Table::Style::NONE);

  tabulate::Format& format() {
    return internalTable_.format();
  }

 private:
  friend std::ostream& operator<<(std::ostream& stream, const Table& table);

  bool showBorders_;
  std::vector<Row> rows_;
  tabulate::Table internalTable_;
};

inline std::ostream& operator<<(std::ostream& stream, const Table& table) {
  return stream << table.internalTable_;
}

} // namespace facebook::fboss::utils
