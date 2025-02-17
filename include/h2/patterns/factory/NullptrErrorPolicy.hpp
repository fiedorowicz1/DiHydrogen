////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2020 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>

namespace h2
{
namespace factory
{
/** \class NullptrErrorPolicy
 *  \brief Returns a nullptr when given an unknown key.
 */
template <typename IdType, class ObjectType>
struct NullptrErrorPolicy
{
  std::unique_ptr<ObjectType> handle_unknown_id(IdType const&) const
  {
    return nullptr;
  }
};  // struct NullptrErrorPolicy

}  // namespace factory
}  // namespace h2
