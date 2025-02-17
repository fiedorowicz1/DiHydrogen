////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#include "h2/meta/Core.hpp"
#include "h2/meta/typelist/Unique.hpp"

using namespace h2::meta;
using namespace h2::meta::tlist;

// Testing Unique
static_assert(EqV<Unique<Empty>, Empty>(), "Unique empty list is empty.");
static_assert(EqV<Unique<TL<int, int, int>>, TL<int>>(),
              "Unique list of the same type.");
static_assert(EqV<Unique<TL<int, float, double, int, float, double>>,
                  TL<int, float, double>>(),
              "Unique an interesting list.");

using TList = Unique<TL<int, float, double, int, float, double>>;
static_assert(EqV<Unique<TList>, TList>(), "Unique a unique list.");
