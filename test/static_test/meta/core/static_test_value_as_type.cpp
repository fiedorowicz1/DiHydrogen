////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#include "h2/meta/core/Eq.hpp"
#include "h2/meta/core/Lazy.hpp"
#include "h2/meta/core/ValueAsType.hpp"

using namespace h2::meta;

static_assert(ValueAsType<int, 13>::value == 13, "ValueAsType holds a value.");
static_assert(EqV<typename ValueAsType<int, 13>::value_type, int>(),
              "ValueAsType has the right typedef set for value_type.");
static_assert(EqV<Force<ValueAsType<int, 13>>, ValueAsType<int, 13>>(),
              "ValueAsType has the right typedef set for type.");
