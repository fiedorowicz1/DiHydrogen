////////////////////////////////////////////////////////////////////////////////
// Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
// DiHydrogen Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////////////

#include "h2/core/sync.hpp"

#include <El.hpp>

#include <unordered_map>

#include "../tensor/utils.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace h2;

TEMPLATE_LIST_TEST_CASE("SyncEvent works", "[sync]", AllDevList)
{
  constexpr Device Dev = TestType::value;

  SyncEvent event{Dev};

  REQUIRE(event.get_device() == Dev);
#ifdef H2_TEST_WITH_GPU
  if constexpr (Dev == Device::GPU)
  {
    REQUIRE(event.get_event<Dev>() != nullptr);
  }
#endif
  REQUIRE_NOTHROW(event.wait_for_this());
}

TEMPLATE_LIST_TEST_CASE("ComputeStream works", "[sync]", AllDevList)
{
  constexpr Device Dev = TestType::value;

  ComputeStream stream{Dev};
  SyncEvent event{Dev};

  REQUIRE(stream.get_device() == Dev);

#ifdef H2_TEST_WITH_GPU
  if constexpr (Dev == Device::GPU)
  {
#if H2_HAS_CUDA
    REQUIRE(stream.get_stream<Device::GPU>() == El::cuda::GetDefaultStream());
#elif H2_HAS_ROCM
    REQUIRE(stream.get_stream<Device::GPU>() == El::rocm::GetDefaultStream());
#endif
  }
#endif

  REQUIRE_NOTHROW(stream.add_sync_point(event));
  REQUIRE_NOTHROW(stream.wait_for(event));

  ComputeStream stream2{Dev};  // Same underlying stream.
  REQUIRE_NOTHROW(stream.wait_for(stream2));

  REQUIRE_NOTHROW(stream.wait_for_this());
}

TEMPLATE_LIST_TEST_CASE("Sync creation routines work", "[sync]", AllDevList)
{
  constexpr Device Dev = TestType::value;

  ComputeStream stream{Dev};
  REQUIRE_NOTHROW([&]() {
    stream = create_new_compute_stream<Dev>();
  }());
  REQUIRE_NOTHROW(destroy_compute_stream<Dev>(stream));
  REQUIRE_NOTHROW([&]() {
    stream = create_new_compute_stream(Dev);
  }());
  REQUIRE_NOTHROW(destroy_compute_stream(stream));

  SyncEvent event{Dev};
  REQUIRE_NOTHROW([&]() {
    event = create_new_sync_event<Dev>();
  }());
  REQUIRE_NOTHROW(destroy_sync_event<Dev>(event));
  REQUIRE_NOTHROW([&]() {
    event = create_new_sync_event(Dev);
  }());
  REQUIRE_NOTHROW(destroy_sync_event(event));
}

TEMPLATE_LIST_TEST_CASE("Copying syncs works", "[sync]", AllDevList)
{
  constexpr Device Dev = TestType::value;

  ComputeStream stream = create_new_compute_stream<Dev>();
  ComputeStream stream_copy = stream;
  REQUIRE(stream.get_stream<Dev>() == stream_copy.get_stream<Dev>());

  SyncEvent event = create_new_sync_event<Dev>();
  SyncEvent event_copy = event;
  REQUIRE(event.get_event<Dev>() == event_copy.get_event<Dev>());
}

TEMPLATE_LIST_TEST_CASE("Self-moving syncs works", "[sync]", AllDevList)
{
  constexpr Device Dev = TestType::value;

  ComputeStream stream = create_new_compute_stream<Dev>();
  auto raw_stream = stream.get_stream<Dev>();
  ComputeStream moved_stream = std::move(stream);
  REQUIRE(moved_stream.get_stream<Dev>() == raw_stream);

  SyncEvent event = create_new_sync_event<Dev>();
  auto raw_event = event.get_event<Dev>();
  SyncEvent moved_event = std::move(event);
  REQUIRE(moved_event.get_event<Dev>() == raw_event);
}

TEMPLATE_LIST_TEST_CASE("Sync helpers work", "[sync]", AllDevList)
{
  constexpr Device Dev = TestType::value;

  ComputeStream stream1{Dev};  // Default stream.
  ComputeStream stream2 = create_new_compute_stream<Dev>();
  ComputeStream stream3 = create_new_compute_stream<Dev>();
  ComputeStream cpu_stream = create_new_compute_stream<Device::CPU>();

  REQUIRE_NOTHROW(all_wait_on_stream(stream1, stream2, stream3, cpu_stream));
  REQUIRE_NOTHROW(stream_wait_on_all(stream1, stream2, stream3, cpu_stream));
}

TEST_CASE("Stream equality works", "[sync]")
{
  ComputeStream stream1 = create_new_compute_stream<Device::CPU>();
  ComputeStream stream2 = create_new_compute_stream<Device::CPU>();

  REQUIRE(stream1 == stream1);
  REQUIRE(stream1 == stream2);
  REQUIRE_FALSE(stream1 != stream2);

  std::unordered_map<ComputeStream, int> map;
  map[stream1] = 1;
  REQUIRE(map.count(stream1) > 0);
  REQUIRE(map.count(stream2) > 0);
}

TEST_CASE("CPU event equality works", "[sync]")
{
  SyncEvent event1 = create_new_sync_event<Device::CPU>();
  SyncEvent event2 = create_new_sync_event<Device::CPU>();

  REQUIRE(event1 == event2);
  REQUIRE(event1 == event2);
  REQUIRE_FALSE(event1 != event2);

  std::unordered_map<SyncEvent, int> map;
  map[event1] = 1;
  REQUIRE(map.count(event1) > 0);
  REQUIRE(map.count(event2) > 0);
}

TEST_CASE("CPU sync El::SyncInfo conversion works", "[sync]")
{
  // Conversion from El:
  El::SyncInfo<El::Device::CPU> sync_info =
    El::CreateNewSyncInfo<El::Device::CPU>();
  ComputeStream stream{Device::CPU};
  REQUIRE_NOTHROW([&]() {
    stream = ComputeStream(sync_info);
  }());
  El::DestroySyncInfo(sync_info);

  // Conversion to El:
  REQUIRE_NOTHROW([&]() {
    sync_info = static_cast<El::SyncInfo<El::Device::CPU>>(stream);
  }());
}

#ifdef H2_TEST_WITH_GPU

TEST_CASE("GPU stream equality works", "[sync]")
{
  ComputeStream stream1 = create_new_compute_stream<Device::GPU>();
  ComputeStream stream2 = create_new_compute_stream<Device::GPU>();
  ComputeStream stream3 = stream1;

  REQUIRE(stream1 == stream1);
  REQUIRE(stream1 == stream3);
  REQUIRE(stream1 != stream2);

  std::unordered_map<ComputeStream, int> map;
  map[stream1] = 1;
  map[stream2] = 2;
  REQUIRE(map.count(stream1) > 0);
  REQUIRE(map.count(stream2) > 0);
  REQUIRE(map[stream1] == 1);
  REQUIRE(map[stream3] == 1);
  REQUIRE(map[stream2] == 2);
}

TEST_CASE("GPU event equality works", "[sync]")
{
  SyncEvent event1 = create_new_sync_event<Device::GPU>();
  SyncEvent event2 = create_new_sync_event<Device::GPU>();
  SyncEvent event3 = event1;

  REQUIRE(event1 == event1);
  REQUIRE(event1 == event3);
  REQUIRE(event1 != event2);

  std::unordered_map<SyncEvent, int> map;
  map[event1] = 1;
  map[event2] = 2;
  REQUIRE(map.count(event1) > 0);
  REQUIRE(map.count(event2) > 0);
  REQUIRE(map[event1] == 1);
  REQUIRE(map[event3] == 1);
  REQUIRE(map[event2] == 2);
}

TEST_CASE("GPU sync El::SyncInfo conversion works", "[sync]")
{
  // Conversion from El:
  El::SyncInfo<El::Device::GPU> sync_info =
    El::CreateNewSyncInfo<Device::GPU>();
  ComputeStream stream{Device::GPU};
  REQUIRE_NOTHROW([&]() {
    stream = ComputeStream(sync_info);
  }());
  REQUIRE(stream.get_stream<Device::GPU>() == sync_info.Stream());
  El::DestroySyncInfo(sync_info);

  // Conversion to El:
  REQUIRE_NOTHROW([&]() {
    sync_info = static_cast<El::SyncInfo<El::Device::GPU>>(stream);
  }());
  REQUIRE(sync_info.Stream() == stream.get_stream<Device::GPU>());
}

TEST_CASE("GPU and CPU syncs interoperate", "[sync]")
{
  ComputeStream gpu_stream{Device::GPU};
  ComputeStream cpu_stream{Device::CPU};
  SyncEvent gpu_event{Device::GPU};
  SyncEvent cpu_event{Device::CPU};

#ifdef H2_DEBUG
  REQUIRE_THROWS(gpu_stream.add_sync_point(cpu_event));
  REQUIRE_THROWS(cpu_stream.add_sync_point(gpu_event));
#endif

  REQUIRE_NOTHROW(gpu_stream.wait_for(cpu_event));
  REQUIRE_NOTHROW(cpu_stream.wait_for(gpu_event));

  REQUIRE_NOTHROW(gpu_stream.wait_for(cpu_stream));
  REQUIRE_NOTHROW(cpu_stream.wait_for(gpu_stream));

  REQUIRE_NOTHROW([&]() {
    auto multi_sync = create_multi_sync(gpu_stream, cpu_stream);
  }());
}

TEST_CASE("Moving GPU syncs clears handles", "[sync]")
{
  ComputeStream stream = create_new_compute_stream<Device::GPU>();
  auto raw_stream = stream.get_stream<Device::GPU>();
  REQUIRE(raw_stream != nullptr);
  ComputeStream stream2 = std::move(stream);
  REQUIRE(stream2.get_stream<Device::GPU>() == raw_stream);
  REQUIRE(stream.get_stream<Device::GPU>() == nullptr);

  SyncEvent event = create_new_sync_event<Device::GPU>();
  auto raw_event = event.get_event<Device::GPU>();
  REQUIRE(raw_event != nullptr);
  SyncEvent event2 = std::move(event);
  REQUIRE(event2.get_event<Device::GPU>() == raw_event);
  REQUIRE(event.get_event<Device::GPU>() == nullptr);
}

#endif  // H2_TEST_WITH_GPU

TEMPLATE_LIST_TEST_CASE("MultiSyncs are sane", "[sync]", AllDevPairsList)
{
  constexpr Device Dev1 = meta::tlist::At<TestType, 0>::value;
  constexpr Device Dev2 = meta::tlist::At<TestType, 1>::value;

  ComputeStream stream1 = create_new_compute_stream<Dev1>();
  ComputeStream stream2 = create_new_compute_stream<Dev2>();

  auto multi_sync = create_multi_sync(stream1, stream2);

  REQUIRE(multi_sync.get_main_stream() == stream1);
  REQUIRE(static_cast<ComputeStream>(multi_sync) == stream1);
  REQUIRE(multi_sync.get_stream<Dev1>() == stream1.get_stream<Dev1>());
}
