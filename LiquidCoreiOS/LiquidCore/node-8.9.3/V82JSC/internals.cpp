//
//  internals.cpp
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

#include <climits>
#include <csignal>
#include <map>
#include <memory>
#include <string>

#include "test/cctest/test-api.h"

#if V8_OS_POSIX
#include <unistd.h>  // NOLINT
#endif

#include "include/v8-util.h"
#include "src/api.h"
#include "src/arguments.h"
#include "src/base/platform/platform.h"
#include "src/code-stubs.h"
#include "src/compilation-cache.h"
#include "src/debug/debug.h"
#include "src/execution.h"
#include "src/futex-emulation.h"
#include "src/heap/incremental-marking.h"
#include "src/lookup.h"
#include "src/objects-inl.h"
#include "src/parsing/preparse-data.h"
#include "src/profiler/cpu-profiler.h"
#include "src/unicode-inl.h"
#include "src/utils.h"
#include "src/vm-state.h"
#include "test/cctest/heap/heap-tester.h"
#include "test/cctest/heap/heap-utils.h"
#include "test/cctest/profiler-extension.h"
#include "test/cctest/print-extension.h"
#include "test/cctest/trace-extension.h"

using namespace v8::internal;

void* Malloced::New(size_t size)
{
    return malloc(size);
}

void Malloced::Delete(void* p)
{
    free(p);
}

Heap::Heap() : external_string_table_(nullptr)
{
    assert(0);
}

StackGuard::StackGuard()
{
    assert(0);
}

Builtins::Builtins()
{
    assert(0);
}

Builtins::~Builtins()
{
    assert(0);
}

ThreadLocalTop::ThreadLocalTop()
{
    printf ("FIXME! ThreadLocalTop()\n");
    context_ = nullptr;
}

AccountingAllocator::AccountingAllocator()
{
    assert(0);
}
AccountingAllocator::~AccountingAllocator()
{
    assert(0);
}
// Gets an empty segment from the pool or creates a new one.
Segment* AccountingAllocator::GetSegment(size_t bytes)
{
    assert(0);
    return nullptr;
}
// Return unneeded segments to either insert them into the pool or release
// them if the pool is already full or memory pressure is high.
void AccountingAllocator::ReturnSegment(Segment* memory)
{
    assert(0);
}

CancelableTaskManager::CancelableTaskManager()
{
    assert(0);
}

void StackGuard::ThreadLocal::Clear()
{
    assert(0);
}

