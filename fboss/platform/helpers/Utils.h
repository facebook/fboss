// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss::platform::helpers {

/*
 * exec command, return contents of stdout and fill in exit status
 */
std::string execCommandUnchecked(const std::string& cmd, int& exitStatus);
/*
 * exec command, return contents of stdout. Throw on command failing (exit
 * status != 0)
 */
std::string execCommand(const std::string& cmd);

void showDeviceInfo();

/*
 * Search Flash Type from flashrom output, e.g. flashrom -p internal
 * If it can not find, return empty string
 */
std::string getFlashType(const std::string& str);

uint64_t nowInSecs();

/*
 * Calculate expression that may contain invalid symbol (for exprtk), e.g. "@",
 * but compatiable with lmsensor style expression
 * and to be replaced by parameter "symbol";
 * or valid variable (for exprtk) in the expression as indicated by
 * parameter "symbol". Those will be replaced by parameter "input" value to
 * calculate the expression.
 * For example: "expression" is "@*0.1", "input" is
 * 100f, invalid symbol is "@" which will get replaced by valid symbol "x",
 * thus the expression will be x*0.1 = 100f*0.1 = 10f
 */
float computeExpression(
    const std::string& expression,
    float input,
    const std::string& symbol = "x");

} // namespace facebook::fboss::platform::helpers
