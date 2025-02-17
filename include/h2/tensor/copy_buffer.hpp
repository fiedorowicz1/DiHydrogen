////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#pragma once

/** @file
 *
 * Low-level routines to copy raw buffers.
 */

#include <h2_config.hpp>

#include "h2/core/sync.hpp"
#include "h2/core/types.hpp"

#include <cstring>
#include <type_traits>

#ifdef H2_HAS_GPU
#include "h2/gpu/memory_utils.hpp"
#endif

namespace h2
{

/**
 * Copy count elements from src to dst.
 *
 * If GPU buffers are involved, this will be asynchronous.
 */
template <typename T>
void copy_buffer(T* dst,
                 ComputeStream const& dst_stream,
                 T const* src,
                 ComputeStream const& src_stream,
                 std::size_t count)
{
  H2_ASSERT_DEBUG(count == 0 || (dst != nullptr && src != nullptr),
                  "Null buffers");
  // TODO: Debug check: Assert buffers do not overlap.
  static_assert(IsH2StorageType_v<T> || std::is_same_v<T*, void*>,
                "Attempt to copy a buffer with a non-storage type");
  Device const src_dev = src_stream.get_device();
  Device const dst_dev = dst_stream.get_device();
  if (src_dev == Device::CPU && dst_dev == Device::CPU)
  {
    if constexpr (std::is_same_v<T*, void*>)
    {
      std::memcpy(dst, src, count);
    }
    else
    {
      std::memcpy(dst, src, count * sizeof(T));
    }
  }
#ifdef H2_HAS_GPU
  else if (src_dev == Device::GPU && dst_dev == Device::GPU)
  {
    auto stream = create_multi_sync(dst_stream, src_stream);
    gpu::mem_copy(dst, src, count, stream.get_stream<Device::GPU>());
  }
  else if (src_dev == Device::CPU && dst_dev == Device::GPU)
  {
    // No sync needed in this case: The CPU is always synchronized and
    // the copy will be enqueued on the destination GPU stream.
    gpu::mem_copy(dst, src, count, dst_stream.get_stream<Device::GPU>());
  }
  else if (src_dev == Device::GPU && dst_dev == Device::CPU)
  {
    // No sync needed: Ditto.
    gpu::mem_copy(dst, src, count, src_stream.get_stream<Device::GPU>());
  }
#endif
  else
  {
    throw H2Exception("Unknown device combination ", src_dev, " and ", dst_dev);
  }
}

}  // namespace h2
