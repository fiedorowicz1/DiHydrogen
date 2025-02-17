////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "h2/tensor/tensor_types.hpp"

#include "tensor_types.hpp"

/** @file
 *
 * Utilities for working with tensors.
 */

namespace h2
{

/**
 * Convert a `ScalarIndexTuple` to a corresponding `IndexRangeTuple`.
 */
constexpr inline IndexRangeTuple
scalar2range_tuple(ScalarIndexTuple const& tuple) H2_NOEXCEPT
{
  IndexRangeTuple ir_tuple(TuplePad<IndexRangeTuple>(tuple.size()));
  for (ScalarIndexTuple::size_type i = 0; i < tuple.size(); ++i)
  {
    ir_tuple[i] = IndexRange(tuple[i]);
  }
  return ir_tuple;
}

/**
 * Return a scalar index tuple denoting the start of an index range.
 *
 * This is the starting point of each index range in the tuple.
 */
constexpr inline ScalarIndexTuple
get_index_range_start(IndexRangeTuple const& coords) H2_NOEXCEPT
{
  ScalarIndexTuple coords_start(TuplePad<ScalarIndexTuple>(coords.size()));
  for (typename IndexRangeTuple::size_type i = 0; i < coords.size(); ++i)
  {
    // No special case for ALL, that starts at 0.
    coords_start[i] = coords[i].start();
  }
  return coords_start;
}

/**
 * Return true if the index range is empty.
 *
 * This occurs when at least one entry in the range is empty or the
 * range itself is empty.
 */
constexpr inline bool
is_index_range_empty(IndexRangeTuple const& coords) H2_NOEXCEPT
{
  return coords.is_empty()
         || any_of(coords, [](typename IndexRangeTuple::type const& c) {
              return c.is_empty();
            });
}

/**
 * Return the shape defined by an index range within a larger shape,
 * eliminating scalar dimensions.
 *
 * If any index ranges in `coords` are empty, the behavior of this is
 * undefined. (However, `coords` itself may be empty, which yields an
 * empty shape.)
 */
constexpr inline ShapeTuple
get_index_range_shape(IndexRangeTuple const& coords,
                      ShapeTuple const& shape) H2_NOEXCEPT
{
  H2_ASSERT_DEBUG(coords.size() <= shape.size(),
                  "coords size (",
                  coords,
                  ") not compatible with shape size (",
                  shape,
                  ")");
  H2_ASSERT_DEBUG(!is_index_range_empty(coords) || coords.is_empty(),
                  "get_index_range_shape does not work with empty ranges");
  ShapeTuple new_shape(TuplePad<ShapeTuple>(shape.size()));
  typename ShapeTuple::size_type j = 0;
  for (typename ShapeTuple::size_type i = 0; i < shape.size(); ++i)
  {
    if (i >= coords.size() || coords[i] == ALL)
    {
      new_shape[j] = shape[i];
      ++j;
    }
    else if (!coords[i].is_scalar())
    {
      new_shape[j] = coords[i].end() - coords[i].start();
      ++j;
    }
  }
  new_shape.set_size(j);
  return new_shape;
}

/**
 * Return true if an index range is contained within a given shape.
 */
constexpr inline bool
is_index_range_contained(IndexRangeTuple const& coords,
                         ShapeTuple const& shape) H2_NOEXCEPT
{
  if (coords.size() > shape.size())
  {
    return false;
  }
  for (typename IndexRangeTuple::size_type i = 0; i < coords.size(); ++i)
  {
    if (coords[i] != ALL
        && (coords[i].start() > shape[i] || coords[i].end() > shape[i]))
    {
      return false;
    }
  }
  return true;
}

/**
 * Return true if two index ranges have a non-empty intersection.
 *
 * The index ranges may not be scalar.
 */
constexpr inline bool
do_index_ranges_intersect(IndexRange const& ir1,
                          IndexRange const& ir2) H2_NOEXCEPT
{
  H2_ASSERT_DEBUG(!ir1.is_scalar() && !ir2.is_scalar(),
                  "Cannot intersect scalar index ranges ",
                  ir1,
                  " and ",
                  ir2);
  return !ir1.is_empty() && !ir2.is_empty()
         && ((ir1 == ALL) || (ir2 == ALL)
             || (ir1.start() < ir2.end() && ir2.start() < ir1.end()));
}

/**
 * Return true if two index ranges have a non-empty intersection.
 *
 * The index ranges may not have scalar entries.
 */
constexpr inline bool
do_index_ranges_intersect(IndexRangeTuple const& ir1,
                          IndexRangeTuple const& ir2) H2_NOEXCEPT
{
  H2_ASSERT_DEBUG(ir1.size() == ir2.size(),
                  "Index ranges ",
                  ir1,
                  " and ",
                  ir2,
                  " must be the same size to intersect");
  for (typename IndexRangeTuple::size_type i = 0; i < ir1.size(); ++i)
  {
    if (!do_index_ranges_intersect(ir1[i], ir2[i]))
    {
      return false;
    }
  }
  return true;
}

/**
 * Return the intersection of of two index ranges.
 *
 * The index ranges must have a non-empty intersection.
 */
constexpr inline IndexRange
intersect_index_ranges(IndexRange const& ir1, IndexRange const& ir2) H2_NOEXCEPT
{
  H2_ASSERT_DEBUG(do_index_ranges_intersect(ir1, ir2),
                  "Index ranges ",
                  ir1,
                  " and ",
                  ir2,
                  " must intersect");
  return IndexRange(std::max(ir1.start(), ir2.start()),
                    std::min(ir1.end(), ir2.end()));
}

/**
 * Return the intersection of of two index ranges.
 *
 * The index ranges must have a non-empty intersection.
 */
constexpr inline IndexRangeTuple
intersect_index_ranges(IndexRangeTuple const& ir1,
                       IndexRangeTuple const& ir2) H2_NOEXCEPT
{
  H2_ASSERT_DEBUG(ir1.size() == ir2.size(),
                  "Index ranges ",
                  ir1,
                  " and ",
                  ir2,
                  " must be the same size to intersect");
  H2_ASSERT_DEBUG(do_index_ranges_intersect(ir1, ir2),
                  "Index ranges ",
                  ir1,
                  " and ",
                  ir2,
                  " must intersect");
  return map_index(ir1, [&ir1, &ir2](IndexRangeTuple::size_type i) {
    return intersect_index_ranges(ir1[i], ir2[i]);
  });
}

/**
 * Return true if the given scalar index is a valid index within shape.
 */
constexpr inline bool is_index_in_shape(ScalarIndexTuple const& idx,
                                        ShapeTuple const& shape) H2_NOEXCEPT
{
  H2_ASSERT_DEBUG(idx.size() == shape.size(),
                  "Scalar indices ",
                  idx,
                  " and shape ",
                  shape,
                  " must be the same size");
  for (typename ScalarIndexTuple::size_type dim = 0; dim < idx.size(); ++dim)
  {
    if (idx[dim] >= shape[dim])
    {
      return false;
    }
  }
  return true;
}

/**
 * Return the next index after idx in a given shape.
 *
 * This gives the next index in the generalized column-major order.
 *
 * If idx is the last index, this will return an index one past the
 * end.
 */
constexpr inline ScalarIndexTuple
next_scalar_index(ScalarIndexTuple const& idx,
                  ShapeTuple const& shape) H2_NOEXCEPT
{
  H2_ASSERT_DEBUG(idx.size() == shape.size(),
                  "Scalar indices ",
                  idx,
                  " and shape ",
                  shape,
                  " must be the same size");
  H2_ASSERT_DEBUG(!idx.is_empty(), "Cannot get next index from an empty index");
  H2_ASSERT_DEBUG(is_index_in_shape(idx, shape),
                  "Cannot get next index from index ",
                  idx,
                  " that is not in shape ",
                  shape);
  ScalarIndexTuple next_idx = idx;
  next_idx[0] += 1;
  for (typename ScalarIndexTuple::size_type dim = 0; dim < idx.size() - 1;
       ++dim)
  {
    if (next_idx[dim] == shape[dim])
    {
      next_idx[dim] = 0;
      next_idx[dim + 1] += 1;
    }
  }
  if (next_idx.back() == shape.back())
  {
    // Went past the end.
    return ScalarIndexTuple::convert_from(shape);
  }
  return next_idx;
}

/**
 * Iterate over an n-dimensional region.
 *
 * The given function f will be called with a `ScalarIndexTuple` for
 * each index position.
 *
 * The iteration is done in the generalized column-major order.
 *
 * @todo In the future, we could specialize for specific dimensions.
 */
template <typename Func>
void for_ndim(ShapeTuple const& shape,
              Func f,
              ScalarIndexTuple const& start = ScalarIndexTuple())
{
  H2_ASSERT_DEBUG(start.is_empty() || start.size() == shape.size(),
                  "Start index ",
                  start,
                  " must be same size as shape ",
                  shape);
  if (shape.is_empty())
  {
    return;
  }
  ScalarIndexTuple coord =
    start.is_empty()
      ? ScalarIndexTuple(TuplePad<ScalarIndexTuple>(shape.size(), 0))
      : start;
  // Upper-bound is the number of elements in shape.
  DataIndexType const ub = product<DataIndexType>(shape);
  // Determine the start by computing the number of skipped indices,
  // essentially by computing an index in a contiguous shape.
  // (Skip this if we know we start from 0.)
  DataIndexType const start_index =
    start.is_empty() ? 0
                     : inner_product<DataIndexType>(
                         coord, prefix_product<DataIndexType>(shape));
  for (DataIndexType i = start_index; i < ub; ++i)
  {
    f(coord);
    coord[0] += 1;
    for (typename ScalarIndexTuple::size_type dim = 0; dim < coord.size() - 1;
         ++dim)
    {
      if (coord[dim] == shape[dim])
      {
        coord[dim] = 0;
        coord[dim + 1] += 1;
      }
    }
  }
}

}  // namespace h2
