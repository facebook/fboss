/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"

#include <folly/Utility.h>

namespace {
using facebook::fboss::CmdGlobalOptions;
using facebook::fboss::utils::Table;

void setColor(tabulate::Format& format, tabulate::Color c) {
  if (CmdGlobalOptions::getInstance()->getColor() == "yes") {
    format.font_color(c);
  }
}

void setStyleImpl(tabulate::Format& format, Table::Style style) {
  switch (style) {
    case Table::Style::NONE:
      setColor(format, tabulate::Color::none);
      return;
    case Table::Style::GOOD:
      setColor(format, tabulate::Color::green);
      return;
    case Table::Style::INFO:
      setColor(format, tabulate::Color::cyan);
      return;
    case Table::Style::WARN:
      setColor(format, tabulate::Color::yellow);
      return;
    case Table::Style::ERROR:
      setColor(format, tabulate::Color::red);
      format.font_style({tabulate::FontStyle::bold});
      return;
  }
  throw std::runtime_error("Invalid Style");
}
} // namespace

namespace facebook::fboss::utils {

Table::Table() {
  showBorders_ = false;
  internalTable_.format().hide_border();
}

Table::Table(bool showBorders) {
  showBorders_ = showBorders;
  showBorders_ ? internalTable_.format().show_border()
               : internalTable_.format().hide_border();
}

Table::Row& Table::setHeader(const std::vector<Table::RowData>& data) {
  if (rows_.size() != 0) {
    throw std::runtime_error(
        "Table::setHeader should be called once, before adding rows");
  }
  auto& row = addRow(data);
  auto formatted =
      row.internalRow_.format().font_style({tabulate::FontStyle::underline});
  if (showBorders_) {
    formatted.show_border();
  }
  return row;
}

Table::Row& Table::addRow(
    const std::vector<Table::RowData>& data,
    Table::Style rowStyle) {
  // transform all items to StyledCells, using StyledCell's implicit constructor
  std::vector<StyledCell> cells;
  cells.reserve(data.size());
  std::transform(
      data.begin(), data.end(), std::back_inserter(cells), [](auto& data) {
        return std::visit([](auto&& cell) { return StyledCell(cell); }, data);
      });

  // transform all items to variant type that tabulate will like
  std::vector<std::variant<std::string, const char*, tabulate::Table>>
      tabulateCells;
  tabulateCells.reserve(data.size());
  std::transform(
      cells.begin(),
      cells.end(),
      std::back_inserter(tabulateCells),
      [](auto& cell) { return cell.getData(); });

  internalTable_.add_row(tabulateCells);
  auto& internalRow = internalTable_.row(rows_.size());
  auto& newRow = rows_.emplace_back(internalRow);

  // Use top border on second row to get the line under header.
  // TODO: figure out why we can't use bottom_border on the header instead
  if (rows_.size() == 2) {
    internalRow.format().corner("-").show_border_top();
  }
  // apply extra styling, if provided
  for (auto i = 0; i < cells.size(); i++) {
    const auto& cell = cells[i];
    auto style =
        cell.getStyle() != Table::Style::NONE ? cell.getStyle() : rowStyle;
    if (style != Table::Style::NONE) {
      newRow.setCellStyle(i, style);
    }
  }
  return newRow;
}

void Table::Row::setStyle(Style style) {
  setStyleImpl(internalRow_.format(), style);
}

void Table::Row::setCellStyle(int cellIndex, Style style) {
  setStyleImpl(internalRow_[cellIndex].format(), style);
}
} // namespace facebook::fboss::utils
