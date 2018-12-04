/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace v8;

HeapStatistics::HeapStatistics()
{
    total_heap_size_ = 0;
    total_heap_size_executable_ = 0;
    total_physical_size_ = 0;
    total_available_size_ = 0;
    used_heap_size_ = 0;
    heap_size_limit_ = 0;
    malloced_memory_ = 0;
    peak_malloced_memory_ = 0;
    does_zap_garbage_ = false;
}

HeapSpaceStatistics::HeapSpaceStatistics()
{
    assert(0);
}

HeapObjectStatistics::HeapObjectStatistics()
{
    assert(0);
}

HeapCodeStatistics::HeapCodeStatistics()
{
    assert(0);
}
