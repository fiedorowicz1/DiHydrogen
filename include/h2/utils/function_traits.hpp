////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#pragma once

/** @file
 *
 * Traits describing functions.
 */

// This partially adapted from
// https://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda


#include "h2/meta/TypeList.hpp"

#include <functional>
#include <cstddef>

namespace h2
{

/**
 * Collect information on a function (anything with operator()).
 */
template <typename T>
struct FunctionTraits
  : public FunctionTraits<decltype(&std::remove_reference_t<T>::operator())>
{};

template <typename Ret, typename... Args>
struct FunctionTraits<Ret(Args...)>
{
  using ArgsList = meta::TypeList<Args...>;
  using RetT = Ret;
  using FuncT = Ret(Args...);

  /** Number of arguments the function takes. */
  static constexpr std::size_t arity = sizeof...(Args);

  /** Whether the function returns a value (a non-void return type). */
  static constexpr bool has_return = !std::is_same_v<RetT, void>;

  /** Access the ith argument type. */
  template <std::size_t i>
  using arg = meta::tlist::At<ArgsList, i>;
};

template <typename Ret, typename... Args>
struct FunctionTraits<Ret (*)(Args...)> : public FunctionTraits<Ret(Args...)>
{};
template <typename FuncT>
struct FunctionTraits<std::function<FuncT>> : public FunctionTraits<FuncT>
{};
template <typename T>
struct FunctionTraits<T&> : public FunctionTraits<T>
{};
template <typename T>
struct FunctionTraits<T const&> : public FunctionTraits<T>
{};
template <typename T>
struct FunctionTraits<T&&> : public FunctionTraits<T>
{};
template <typename T>
struct FunctionTraits<T const&&> : public FunctionTraits<T>
{};
template <typename T>
struct FunctionTraits<T*> : public FunctionTraits<T>
{};
template <typename T>
struct FunctionTraits<T const*> : public FunctionTraits<T>
{};

template <typename Class, typename Ret, typename... Args>
struct FunctionTraits<Ret (Class::*)(Args...)>
  : public FunctionTraits<Ret(Args...)>
{
  using ClassT = Class;
};
template <typename Class, typename Ret, typename... Args>
struct FunctionTraits<Ret (Class::*)(Args...) const>
  : public FunctionTraits<Ret(Args...)>
{
  using ClassT = Class;
};

}  // namespace h2
