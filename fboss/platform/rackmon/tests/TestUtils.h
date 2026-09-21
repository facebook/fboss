// Suppress deprecation warning for ciso646 header included by gtest
#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#pragma GCC diagnostic pop
