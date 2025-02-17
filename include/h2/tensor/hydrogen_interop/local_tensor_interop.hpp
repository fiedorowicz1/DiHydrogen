////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "h2/tensor/tensor.hpp"
#include "h2/utils/As.hpp"

#include <El.hpp>

#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "interop_utils.hpp"

namespace h2
{
namespace internal
{

template <Device D, typename BufferT, typename T>
auto as_h_mat_impl(BufferT buf,
                   Tensor<T> const& tensor) -> El::Matrix<T, HydrogenDevice<D>>
{
  // Enforce usage constraint
  static_assert(std::is_same_v<std::decay_t<std::remove_pointer_t<BufferT>>, T>,
                "BufferT must be T* or T const*");

  using MatrixType = El::Matrix<T, HydrogenDevice<D>>;
  using h_size_type = typename MatrixType::size_type;
  if (tensor.is_empty())
    throw std::runtime_error("Cannot convert empty tensor to El::Matrix");
  if (tensor.ndim() > 1 && !is_chw_packed(tensor))
    throw std::runtime_error("No-copy conversion only supported for "
                             "fully-packed or chw-packed tensors");
  if (tensor.ndim() == 1)
  {
    auto constexpr h_one = h_size_type{1};
    auto const nelems = safe_as<h_size_type>(tensor.numel());
    auto const elem_stride = safe_as<h_size_type>(tensor.stride(0));
    if (elem_stride == h_one)
      return MatrixType{nelems, h_one, buf, nelems};
    else
      return MatrixType{h_one, nelems, buf, elem_stride};
  }
  auto const& shape = tensor.shape();
  auto const& strides = tensor.strides();
  auto const width = safe_as<h_size_type>(shape.back());
  auto const height = safe_as<h_size_type>(product<std::uint64_t>(init(shape)));
  auto const ldim = safe_as<h_size_type>(strides.back());
  return MatrixType{height, width, buf, ldim};
}

template <typename BufferT, typename T, hydrogen::Device D>
auto as_h2_tensor_impl(BufferT buf, El::Matrix<T, D> const& matrix)
{
  // Enforce usage constraint
  static_assert(std::is_same_v<std::decay_t<std::remove_pointer_t<BufferT>>, T>,
                "BufferT must be T* or T const*");

  using TensorType = Tensor<T>;
  if (matrix.IsEmpty())
    throw std::runtime_error("Cannot convert empty matrix to Tensor");

  auto const m = safe_as<DimType>(matrix.Height());
  auto const n = safe_as<DimType>(matrix.Width());
  auto const ldim = safe_as<DataIndexType>(matrix.LDim());
  if (n == DimType{1})  // Column vector
  {
    return TensorType{H2Device<D>,
                      buf,
                      {m},
                      {DT::Any},
                      {as<DataIndexType>(1)},
                      ComputeStream(get_sync_info(matrix))};
  }
  else if (m == DimType{1})  // Row vector
  {
    return TensorType{H2Device<D>,
                      buf,
                      {n},
                      {DT::Any},
                      {ldim},
                      ComputeStream(get_sync_info(matrix))};
  }
  return TensorType{H2Device<D>,
                    buf,
                    {m, n},
                    {DT::Any, DT::Any},
                    {as<DataIndexType>(1), ldim},
                    ComputeStream(get_sync_info(matrix))};
}
}  // namespace internal

/** @brief View an H2 Tensor as a Hydrogen matrix.
 *
 *  This creates a weak view of certain tensors in Hydrogen matrix
 *  format. The tensors must either be rank-1 or be at least
 *  CHW-packed (in cuDNN's nomenclature); that is, at least the N-1
 *  fastest varying indices of a rank-N tensor must be fully packed.
 *
 *  For a general rank-N (N>1) tensor, the slowest-varying index
 *  becomes the width of the matrix, and the product of the N-1
 *  fastest varying indices becomes the height. The "leading
 *  dimension" will be the stride of the slowest-varying index.
 *
 *  Rank-1 tensors can always be viewed. Packed rank-1 tensors will be
 *  viewed as "column vectors" (height == tensor.shape(0), width == 1,
 *  ldim == height), and non-packed rank-1 tensors will be viewed as
 *  "row vectors" (height == 1, width == tensor.shape(0), ldim ==
 *  tensor.stride(0)).
 *
 *  "Empty" tensors are not viewable as their data pointer is not
 *  considered valid.
 *
 *  It is important to note that the reference count on the internal
 *  H2 data structure is not affected by this call -- hence, "weak
 *  view". Users of this API are responsible for ensuring data
 *  consistency of the tensor data for the lifetime of the Hydrogen
 *  view.
 *
 *  @tparam T (Inferred) Data type of tensor elements
 *  @tparam D (Inferred) Device residency of tensor data
 *
 *  @param[in] tensor The tensor to view in Hydrogen format.
 *
 *  @returns A "locked" view of the tensor data in Hydrogen format
 *
 *  @throws std::runtime_error Thrown when the source tensor cannot be
 *                             viewed in Hydrogen format.
 */
template <Device D, typename T>
auto as_h_mat(Tensor<T> const& tensor) -> El::Matrix<T, HydrogenDevice<D>>
{
  return internal::as_h_mat_impl<D>(tensor.const_data(), tensor);
}

/** @brief View an H2 Tensor as a Hydrogen matrix.
 *
 *  This creates a weak view of certain tensors in Hydrogen matrix
 *  format. The tensors must either be rank-1 or be at least
 *  CHW-packed (in cuDNN's nomenclature); that is, at least the N-1
 *  fastest varying indices of a rank-N tensor must be fully packed.
 *
 *  For a general rank-N (N>1) tensor, the slowest-varying index
 *  becomes the width of the matrix, and the product of the N-1
 *  fastest varying indices becomes the height. The "leading
 *  dimension" will be the stride of the slowest-varying index.
 *
 *  Rank-1 tensors can always be viewed. Packed rank-1 tensors will be
 *  viewed as "column vectors" (height == tensor.shape(0), width == 1,
 *  ldim == height), and non-packed rank-1 tensors will be viewed as
 *  "row vectors" (height == 1, width == tensor.shape(0), ldim ==
 *  tensor.stride(0)).
 *
 *  "Empty" tensors are not viewable as their data pointer is not
 *  considered valid.
 *
 *  It is important to note that the reference count on the internal
 *  H2 data structure is not affected by this call -- hence, "weak
 *  view". Users of this API are responsible for ensuring data
 *  consistency of the tensor data for the lifetime of the Hydrogen
 *  view.
 *
 *  @tparam T (Inferred) Data type of tensor elements
 *  @tparam D (Inferred) Device residency of tensor data
 *
 *  @param[in] tensor The tensor to view in Hydrogen format.
 *
 *  @returns A mutable view of the tensor data in Hydrogen format
 *
 *  @throws std::runtime_error Thrown when the source tensor cannot be
 *                             viewed in Hydrogen format.
 */
template <Device D, typename T>
auto as_h_mat(Tensor<T>& tensor) -> El::Matrix<T, HydrogenDevice<D>>
{
  return internal::as_h_mat_impl<D>(tensor.data(), tensor);
}

/** @brief View a Hydrogen matrix as an H2 Tensor
 *
 *  This creates a weak view of a Hydrogen matrix in H2 tensor format.
 *  All non-empty Hydrogen matrices are viewable in this format.
 *
 *  Special behavior applies when the matrix is a "vector", i.e., it
 *  has either unit height or unit width. In both cases, the resulting
 *  tensor will be of rank 1. In the case of a "column vector" (`width
 *  == 1`), the output tensor will be fully-packed (`tensor.stride(0)
 *  == 1`). In the case of a "row vector", the output tensor will be
 *  strided according to the "leading dimension" of the matrix
 *  (`tensor.stride(0) == matrix.LDim()`).
 *
 *  "Empty" matrices are not viewable as their data pointer is not
 *  considered valid.
 *
 *  @tparam T (Inferred) Data type of matrix elements
 *  @tparam D (Inferred) Device residency of matrix data
 *
 *  @param[in] matrix The matrix to view in H2 tensor format.
 *
 *  @returns A "Const" view of the matrix data in H2 tensor format
 *
 *  @throws std::runtime_error Thrown when the source matrix cannot be
 *                             viewed in H2 tensor format.
 */
template <typename T, hydrogen::Device D>
auto as_h2_tensor(El::Matrix<T, D> const& matrix) -> Tensor<T>
{
  return internal::as_h2_tensor_impl(matrix.LockedBuffer(), matrix);
}

/** @brief View a Hydrogen matrix as an H2 Tensor
 *
 *  This creates a weak view of a Hydrogen matrix in H2 tensor format.
 *  All non-empty Hydrogen matrices are viewable in this format.
 *
 *  Special behavior applies when the matrix is a "vector", i.e., it
 *  has either unit height or unit width. In both cases, the resulting
 *  tensor will be of rank 1. In the case of a "column vector" (`width
 *  == 1`), the output tensor will be fully-packed (`tensor.stride(0)
 *  == 1`). In the case of a "row vector", the output tensor will be
 *  strided according to the "leading dimension" of the matrix
 *  (`tensor.stride(0) == matrix.LDim()`).
 *
 *  "Empty" matrices are not viewable as their data pointer is not
 *  considered valid.
 *
 *  @tparam T (Inferred) Data type of matrix elements
 *  @tparam D (Inferred) Device residency of matrix data
 *
 *  @param[in] matrix The matrix to view in H2 tensor format.
 *
 *  @returns A "Mutable" view of the matrix data in H2 tensor format
 *
 *  @throws std::runtime_error Thrown when the source matrix cannot be
 *                             viewed in H2 tensor format.
 */
template <typename T, hydrogen::Device D>
auto as_h2_tensor(El::Matrix<T, D>& matrix) -> Tensor<T>
{
  return internal::as_h2_tensor_impl(matrix.Buffer(), matrix);
}

}  // namespace h2
