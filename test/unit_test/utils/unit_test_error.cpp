////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#include "h2/utils/Error.hpp"

#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace h2;

TEST_CASE("H2BaseException works", "[utilities][error]")
{
  try
  {
    throw H2ExceptionBase("foo");
  }
  catch (H2ExceptionBase const& e)
  {
    // May or may not collect a backtrace.
    REQUIRE_THAT(e.what(), Catch::Matchers::StartsWith("foo"));
  }

  try
  {
    throw H2ExceptionBase("foo", SaveBacktrace);
  }
  catch (H2ExceptionBase const& e)
  {
    REQUIRE_THAT(e.what(), Catch::Matchers::StartsWith("foo\nStack trace:\n"));
  }

  try
  {
    throw H2ExceptionBase("foo", NoSaveBacktrace);
  }
  catch (H2ExceptionBase const& e)
  {
    REQUIRE_THAT(e.what(), Catch::Matchers::StartsWith("foo"));
    REQUIRE_THAT(e.what(), !Catch::Matchers::ContainsSubstring("Stack trace:"));
  }
}

TEST_CASE("H2FatalException works", "[utilities][error]")
{
  try
  {
    throw H2FatalException("foo", 1234);
  }
  catch (H2ExceptionBase const& e)
  {
    REQUIRE_THAT(e.what(),
                 Catch::Matchers::StartsWith("foo1234\nStack trace:\n"));
  }
}
