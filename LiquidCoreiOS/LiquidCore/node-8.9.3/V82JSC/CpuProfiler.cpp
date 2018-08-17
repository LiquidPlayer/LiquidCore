//
//  CpuProfiler.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "V82JSC.h"

using namespace v8;

/**
 * Interface for controlling CPU profiling. Instance of the
 * profiler can be created using v8::CpuProfiler::New method.
 */

/**
 * Creates a new CPU profiler for the |isolate|. The isolate must be
 * initialized. The profiler object must be disposed after use by calling
 * |Dispose| method.
 */
CpuProfiler* CpuProfiler::New(Isolate* isolate)
{
    assert(0);
    return nullptr;
}
    
/**
 * Disposes the CPU profiler object.
 */
void CpuProfiler::Dispose()
{
    assert(0);
}

/**
 * Changes default CPU profiler sampling interval to the specified number
 * of microseconds. Default interval is 1000us. This method must be called
 * when there are no profiles being recorded.
 */
void CpuProfiler::SetSamplingInterval(int us)
{
    assert(0);
}

/**
 * Starts collecting CPU profile. Title may be an empty string. It
 * is allowed to have several profiles being collected at
 * once. Attempts to start collecting several profiles with the same
 * title are silently ignored. While collecting a profile, functions
 * from all security contexts are included in it. The token-based
 * filtering is only performed when querying for a profile.
 *
 * |record_samples| parameter controls whether individual samples should
 * be recorded in addition to the aggregated tree.
 */
void CpuProfiler::StartProfiling(Local<String> title, bool record_samples)
{
    assert(0);
}

/**
 * Stops collecting CPU profile with a given title and returns it.
 * If the title given is empty, finishes the last profile started.
 */
CpuProfile* CpuProfiler::StopProfiling(Local<String> title)
{
    assert(0);
    return nullptr;
}

/**
 * Force collection of a sample. Must be called on the VM thread.
 * Recording the forced sample does not contribute to the aggregated
 * profile statistics.
 */
void CpuProfiler::CollectSample()
{
    assert(0);
}

/**
 * Tells the profiler whether the embedder is idle.
 */
void CpuProfiler::SetIdle(bool is_idle)
{
    assert(0);
}
