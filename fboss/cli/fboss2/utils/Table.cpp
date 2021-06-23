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

#include <folly/Utility.h>

namespace facebook::fboss::utils {

Table::Table() {
  internalTable_.format().hide_border();
}

Table::Row& Table::setHeader(const std::vector<std::string>& data) {
  if (rows_.size() != 0) {
    throw std::runtime_error(
        "Table::setHeader should be called once, before adding rows");
  }
  auto& row = addRow(data);
  row.internalRow_.format().font_style({tabulate::FontStyle::underline});
  return row;
}

Table::Row& Table::addRow(const std::vector<std::string>& data) {
  // transform all items to variant type that tabulate will like
  std::vector<std::variant<std::string, const char*, tabulate::Table>>
      tableCells;
  std::transform(
      data.begin(),
      data.end(),
      std::back_inserter(tableCells),
      folly::identity);

  internalTable_.add_row(tableCells);
  auto& internalRow = internalTable_.row(rows_.size());

  // Use top border on second row to get the line under header.
  // TODO: figure out why we can't use bottom_border on the header instead
  if (rows_.size() == 2) {
    internalRow.format().corner("-").show_border_top();
  }

  return rows_.emplace_back(internalRow);
}

} // namespace facebook::fboss::utils
